/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1999-2000 Netscape Communications Corporation.  All Rights
 * Reserved.
 * 
 * Contributor(s):
 * Norris Boyd
 * Mitch Stoltz
 */

#include "nsBasePrincipal.h"
#include "nsScriptSecurityManager.h"
#include "nsString.h"
#include "plstr.h"
#include "nsIPref.h"

//////////////////////////

nsBasePrincipal::nsBasePrincipal()
    : mCapabilities(nsnull), mPrefName(nsnull)
{
}

PR_STATIC_CALLBACK(PRBool)
deleteElement(void* aElement, void *aData)
{
    nsHashtable *ht = (nsHashtable *) aElement;
    delete ht;
    return PR_TRUE;
}

nsBasePrincipal::~nsBasePrincipal(void)
{
    mAnnotations.EnumerateForwards(deleteElement, nsnull);
    delete mCapabilities;
    if (mPrefName)
        Recycle(mPrefName);
}

NS_IMETHODIMP
nsBasePrincipal::GetJSPrincipals(JSPrincipals **jsprin)
{
    if (mJSPrincipals.nsIPrincipalPtr == nsnull) {
        mJSPrincipals.nsIPrincipalPtr = this;
        // No need for a ADDREF since it is a self-reference
    }
    *jsprin = &mJSPrincipals;
    JSPRINCIPALS_HOLD(cx, *jsprin);
    return NS_OK;
}

const char
nsBasePrincipal::Invalid[] = "Invalid";

NS_IMETHODIMP 
nsBasePrincipal::CanEnableCapability(const char *capability, PRInt16 *result)
{
    if (!mCapabilities) {
        *result = nsIPrincipal::ENABLE_UNKNOWN;
        return NS_OK;
    }
    else // If this principal is marked invalid, can't enable any capabilities
    {
        nsStringKey invalidKey(Invalid);
        if (mCapabilities->Exists(&invalidKey))
        {
           *result = nsIPrincipal::ENABLE_DENIED;
           return NS_OK;
        }
    }

    const char *start = capability;
    *result = nsIPrincipal::ENABLE_GRANTED;
    for(;;) {
        const char *space = PL_strchr(start, ' ');
        int len = space ? space - start : nsCRT::strlen(start);
        nsCAutoString capString(start, len);
        nsStringKey key(capString);
        PRInt16 value = (PRInt16)(PRInt32)mCapabilities->Get(&key);
        if (value == 0)
            value = nsIPrincipal::ENABLE_UNKNOWN;
        if (value < *result)
            *result = value;
        if (!space)
            return NS_OK;
        start = space + 1;
    }
}

NS_IMETHODIMP 
nsBasePrincipal::SetCanEnableCapability(const char *capability, 
                                        PRInt16 canEnable)
{
    if (!mCapabilities) {
        mCapabilities = new nsHashtable(7);
        if (!mCapabilities)
            return NS_ERROR_OUT_OF_MEMORY;
    }
    else // If this principal is marked invalid, can't enable any capabilities
    {
        nsStringKey invalidKey(Invalid);
        if (mCapabilities->Exists(&invalidKey))
            return NS_OK;
    }

    if (PL_strcmp(capability, Invalid) == 0)
        mCapabilities->Reset();

    const char *start = capability;
    for(;;) {
        const char *space = PL_strchr(start, ' ');
        int len = space ? space - start : nsCRT::strlen(start);
        nsCAutoString capString(start, len);
        nsStringKey key(capString);
        mCapabilities->Put(&key, (void *) canEnable);
        if (!space)
            return NS_OK;
        start = space + 1;
    }
}

NS_IMETHODIMP 
nsBasePrincipal::IsCapabilityEnabled(const char *capability, void *annotation, 
                                     PRBool *result)
{
    *result = PR_FALSE;
    nsHashtable *ht = (nsHashtable *) annotation;
    if (!ht) {
        return NS_OK;
    }
    const char *start = capability;
    for(;;) {
        const char *space = PL_strchr(start, ' ');
        int len = space ? space - start : nsCRT::strlen(start);
        nsCAutoString capString(start, len);
        nsStringKey key(capString);
        *result = (ht->Get(&key) == (void *) AnnotationEnabled);
        if (!*result) {
            // If any single capability is not enabled, then return false.
            return NS_OK;
        }
        if (!space)
            return NS_OK;
        start = space + 1;
    }
    return NS_OK;
}

NS_IMETHODIMP 
nsBasePrincipal::EnableCapability(const char *capability, void **annotation) 
{
    return SetCapability(capability, annotation, AnnotationEnabled);
}

NS_IMETHODIMP 
nsBasePrincipal::DisableCapability(const char *capability, void **annotation) 
{
    return SetCapability(capability, annotation, AnnotationDisabled);
}

NS_IMETHODIMP 
nsBasePrincipal::RevertCapability(const char *capability, void **annotation) 
{
    if (*annotation) {
        nsHashtable *ht = (nsHashtable *) *annotation;
        const char *start = capability;
        for(;;) {
            const char *space = PL_strchr(start, ' ');
            int len = space ? space - start : nsCRT::strlen(start);
            nsCAutoString capString(start, len);
            nsStringKey key(capString);
            ht->Remove(&key);
            if (!space)
                return NS_OK;
            start = space + 1;
        }
    }
    return NS_OK;
}    

NS_IMETHODIMP
nsBasePrincipal::SetCapability(const char *capability, void **annotation,
                               AnnotationValue value) 
{
    if (*annotation == nsnull) {
        *annotation = new nsHashtable(5);
        if (!*annotation)
             return NS_ERROR_OUT_OF_MEMORY;
        // This object owns its annotations. Save them so we can release
        // them when we destroy this object.
        mAnnotations.AppendElement(*annotation);
    }

    const char *start = capability;
    for(;;) {
        const char *space = PL_strchr(start, ' ');
        int len = space ? space - start : nsCRT::strlen(start);
        nsCAutoString capString(start, len);
        nsStringKey key(capString);
        nsHashtable *ht = (nsHashtable *) *annotation;
        ht->Put(&key, (void *) value);
        if (!space)
            return NS_OK;
        start = space + 1;
    }
}

int nsBasePrincipal::mCapabilitiesOrdinal = 0;

nsresult
nsBasePrincipal::InitFromPersistent(const char* aPrefName, const char* aID, 
                                    const char* aGrantedList, const char* aDeniedList)
{
    //-- Empty the capability table
    if (mCapabilities)
        mCapabilities->Reset();

    //-- Save the preference name
    mPrefName = PL_strdup(aPrefName);
    if (!mPrefName)
        return NS_ERROR_OUT_OF_MEMORY;

    const char* ordinalBegin = PL_strpbrk(aPrefName, "1234567890");
    if (ordinalBegin) {
        int  n = atoi(ordinalBegin);
         if (mCapabilitiesOrdinal <= n)
            mCapabilitiesOrdinal = n+1;
    }
    
    //-- Store the capabilities
    if (aGrantedList)
        if(NS_FAILED(SetCanEnableCapability(aGrantedList, nsIPrincipal::ENABLE_GRANTED))) 
            return NS_ERROR_FAILURE;
    if (aDeniedList)
        if(NS_FAILED(SetCanEnableCapability(aDeniedList, nsIPrincipal::ENABLE_DENIED))) 
            return NS_ERROR_FAILURE;
    return NS_OK;
}

struct CapabilityList
{
    nsCString* granted;
    nsCString* denied;
};

PR_STATIC_CALLBACK(PRBool)
AppendCapability(nsHashKey *aKey, void *aData, void *capListPtr)
{
    CapabilityList* capList = (CapabilityList*)capListPtr;
    PRInt16 value = (PRInt16)(PRInt32)aData;
    if (value == nsIPrincipal::ENABLE_GRANTED)
    {
        capList->granted->AppendWithConversion(((nsStringKey *) aKey)->GetString());
        capList->granted->Append(' ');
    }
    else if (value == nsIPrincipal::ENABLE_DENIED)
    {
        capList->denied->AppendWithConversion(((nsStringKey *) aKey)->GetString());
        capList->denied->Append(' ');
    }
    return PR_TRUE;
}

NS_IMETHODIMP 
nsBasePrincipal::GetPreferences(char** aPrefName, char** aID, 
                                char** aGrantedList, char** aDeniedList)
{
    //-- Preference name
    *aPrefName = nsCRT::strdup(mPrefName);
    if (!aPrefName)
        return NS_ERROR_OUT_OF_MEMORY;

    //-- ID
    if (NS_FAILED(ToString(aID)))
        return NS_ERROR_FAILURE;

    //-- Capabilities
    *aGrantedList = nsnull;
    *aDeniedList = nsnull;
    if (mCapabilities) {
        nsCAutoString grantedListStr;
        nsCAutoString deniedListStr;
        CapabilityList* capList = new CapabilityList();
        capList->granted = &grantedListStr;
        capList->denied = &deniedListStr;        
        mCapabilities->Enumerate(AppendCapability, (void*)capList);
        if (grantedListStr.Length() > 0)
        {
            grantedListStr.Truncate(grantedListStr.Length()-1);
            *aGrantedList = grantedListStr.ToNewCString();
            if (!*aGrantedList) return NS_ERROR_OUT_OF_MEMORY;
        }
        if (deniedListStr.Length() > 0)
        {
            deniedListStr.Truncate(deniedListStr.Length()-1);
            *aDeniedList = deniedListStr.ToNewCString();
            if (!*aDeniedList) return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return NS_OK;
}
