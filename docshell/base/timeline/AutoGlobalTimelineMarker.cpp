/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/AutoGlobalTimelineMarker.h"

#include "TimelineConsumers.h"
#include "MainThreadUtils.h"

namespace mozilla {

AutoGlobalTimelineMarker::AutoGlobalTimelineMarker(const char* aName
                                                   MOZ_GUARD_OBJECT_NOTIFIER_PARAM_IN_IMPL)
  : mName(aName)
{
  MOZ_GUARD_OBJECT_NOTIFIER_INIT;
  MOZ_ASSERT(NS_IsMainThread());

  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  TimelineConsumers::AddMarkerForAllObservedDocShells(mName, TRACING_INTERVAL_START);
}

AutoGlobalTimelineMarker::~AutoGlobalTimelineMarker()
{
  if (TimelineConsumers::IsEmpty()) {
    return;
  }

  TimelineConsumers::AddMarkerForAllObservedDocShells(mName, TRACING_INTERVAL_END);
}

} // namespace mozilla
