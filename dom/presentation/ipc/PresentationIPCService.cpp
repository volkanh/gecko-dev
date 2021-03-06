/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 ts=8 et ft=cpp : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/PPresentation.h"
#include "mozilla/ipc/InputStreamUtils.h"
#include "nsIPresentationListener.h"
#include "PresentationCallbacks.h"
#include "PresentationChild.h"
#include "PresentationIPCService.h"

using namespace mozilla;
using namespace mozilla::dom;

namespace {

PresentationChild* sPresentationChild;

} // anonymous

NS_IMPL_ISUPPORTS(PresentationIPCService, nsIPresentationService)

PresentationIPCService::PresentationIPCService()
{
  ContentChild* contentChild = ContentChild::GetSingleton();
  if (NS_WARN_IF(!contentChild)) {
    return;
  }
  sPresentationChild = new PresentationChild(this);
  NS_WARN_IF(!contentChild->SendPPresentationConstructor(sPresentationChild));
}

/* virtual */
PresentationIPCService::~PresentationIPCService()
{
  mListeners.Clear();
  mSessionListeners.Clear();
  sPresentationChild = nullptr;
}

NS_IMETHODIMP
PresentationIPCService::StartSession(const nsAString& aUrl,
                                     const nsAString& aSessionId,
                                     const nsAString& aOrigin,
                                     nsIPresentationServiceCallback* aCallback)
{
  return SendRequest(aCallback,
                     StartSessionRequest(nsAutoString(aUrl), nsAutoString(aSessionId), nsAutoString(aOrigin)));
}

NS_IMETHODIMP
PresentationIPCService::SendSessionMessage(const nsAString& aSessionId,
                                           nsIInputStream* aStream)
{
  MOZ_ASSERT(!aSessionId.IsEmpty());
  MOZ_ASSERT(aStream);

  mozilla::ipc::OptionalInputStreamParams stream;
  nsTArray<mozilla::ipc::FileDescriptor> fds;
  SerializeInputStream(aStream, stream, fds);
  MOZ_ASSERT(fds.IsEmpty());

  return SendRequest(nullptr, SendSessionMessageRequest(nsAutoString(aSessionId), stream));
}

NS_IMETHODIMP
PresentationIPCService::Terminate(const nsAString& aSessionId)
{
  MOZ_ASSERT(!aSessionId.IsEmpty());

  return SendRequest(nullptr, TerminateRequest(nsAutoString(aSessionId)));
}

nsresult
PresentationIPCService::SendRequest(nsIPresentationServiceCallback* aCallback,
                                    const PresentationRequest& aRequest)
{
  if (sPresentationChild) {
    PresentationRequestChild* actor = new PresentationRequestChild(aCallback);
    NS_WARN_IF(!sPresentationChild->SendPPresentationRequestConstructor(actor, aRequest));
  }
  return NS_OK;
}

NS_IMETHODIMP
PresentationIPCService::RegisterListener(nsIPresentationListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aListener);

  mListeners.AppendElement(aListener);
  if (sPresentationChild) {
    NS_WARN_IF(!sPresentationChild->SendRegisterHandler());
  }
  return NS_OK;
}

NS_IMETHODIMP
PresentationIPCService::UnregisterListener(nsIPresentationListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aListener);

  mListeners.RemoveElement(aListener);
  if (sPresentationChild) {
    NS_WARN_IF(!sPresentationChild->SendUnregisterHandler());
  }
  return NS_OK;
}

NS_IMETHODIMP
PresentationIPCService::RegisterSessionListener(const nsAString& aSessionId,
                                                nsIPresentationSessionListener* aListener)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aListener);

  mSessionListeners.Put(aSessionId, aListener);
  if (sPresentationChild) {
    NS_WARN_IF(!sPresentationChild->SendRegisterSessionHandler(nsAutoString(aSessionId)));
  }
  return NS_OK;
}

NS_IMETHODIMP
PresentationIPCService::UnregisterSessionListener(const nsAString& aSessionId)
{
  MOZ_ASSERT(NS_IsMainThread());

  mSessionListeners.Remove(aSessionId);
  if (sPresentationChild) {
    NS_WARN_IF(!sPresentationChild->SendUnregisterSessionHandler(nsAutoString(aSessionId)));
  }
  return NS_OK;
}

nsresult
PresentationIPCService::NotifySessionStateChange(const nsAString& aSessionId,
                                                 uint16_t aState)
{
  nsCOMPtr<nsIPresentationSessionListener> listener;
  if (NS_WARN_IF(!mSessionListeners.Get(aSessionId, getter_AddRefs(listener)))) {
    return NS_OK;
  }

  return listener->NotifyStateChange(aSessionId, aState);
}

nsresult
PresentationIPCService::NotifyMessage(const nsAString& aSessionId,
                                      const nsACString& aData)
{
  nsCOMPtr<nsIPresentationSessionListener> listener;
  if (NS_WARN_IF(!mSessionListeners.Get(aSessionId, getter_AddRefs(listener)))) {
    return NS_OK;
  }

  return listener->NotifyMessage(aSessionId, aData);
}

nsresult
PresentationIPCService::NotifyAvailableChange(bool aAvailable)
{
  nsTObserverArray<nsCOMPtr<nsIPresentationListener> >::ForwardIterator iter(mListeners);
  while (iter.HasMore()) {
    nsIPresentationListener* listener = iter.GetNext();
    NS_WARN_IF(NS_FAILED(listener->NotifyAvailableChange(aAvailable)));
  }

  return NS_OK;
}

NS_IMETHODIMP
PresentationIPCService::GetExistentSessionIdAtLaunch(nsAString& aSessionId)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsAutoString sessionId(aSessionId);
  NS_WARN_IF(!sPresentationChild->SendGetExistentSessionIdAtLaunch(&sessionId));
  aSessionId = sessionId;

  return NS_OK;
}

nsresult
PresentationIPCService::NotifyReceiverReady(const nsAString& aSessionId)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_WARN_IF(!sPresentationChild->SendNotifyReceiverReady(nsAutoString(aSessionId)));
  return NS_OK;
}

void
PresentationIPCService::NotifyPresentationChildDestroyed()
{
  sPresentationChild = nullptr;
}

nsresult
PresentationIPCService::MonitorResponderLoading(const nsAString& aSessionId,
                                                nsIDocShell* aDocShell)
{
  mCallback = new PresentationResponderLoadingCallback(aSessionId);
  return mCallback->Init(aDocShell);
}
