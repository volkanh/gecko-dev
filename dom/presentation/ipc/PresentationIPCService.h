/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_PresentationIPCService_h
#define mozilla_dom_PresentationIPCService_h

#include "nsIPresentationService.h"
#include "nsRefPtrHashtable.h"
#include "nsTObserverArray.h"

class nsIDocShell;

namespace mozilla {
namespace dom {

class PresentationRequest;
class PresentationResponderLoadingCallback;

class PresentationIPCService final : public nsIPresentationService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPRESENTATIONSERVICE

  PresentationIPCService();

  nsresult NotifyAvailableChange(bool aAvailable);

  nsresult NotifySessionStateChange(const nsAString& aSessionId,
                                    uint16_t aState);

  nsresult NotifyMessage(const nsAString& aSessionId,
                         const nsACString& aData);

  void NotifyPresentationChildDestroyed();

  nsresult MonitorResponderLoading(const nsAString& aSessionId,
                                   nsIDocShell* aDocShell);

private:
  virtual ~PresentationIPCService();
  nsresult SendRequest(nsIPresentationServiceCallback* aCallback,
                       const PresentationRequest& aRequest);

  nsTObserverArray<nsCOMPtr<nsIPresentationListener> > mListeners;
  nsRefPtrHashtable<nsStringHashKey, nsIPresentationSessionListener> mSessionListeners;
  nsRefPtr<PresentationResponderLoadingCallback> mCallback;
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_PresentationIPCService_h
