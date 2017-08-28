/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef InspectorMemoryAgent_h
#define InspectorMemoryAgent_h

#if ENABLE(RESOURCE_USAGE)

#include "InspectorWebAgentBase.h"
#include "ResourceUsageData.h"
#include <inspector/InspectorBackendDispatchers.h>
#include <inspector/InspectorFrontendDispatchers.h>

namespace WebCore {

typedef String ErrorString;

class InspectorMemoryAgent final : public InspectorAgentBase, public Inspector::MemoryBackendDispatcherHandler {
    WTF_MAKE_NONCOPYABLE(InspectorMemoryAgent);
    WTF_MAKE_FAST_ALLOCATED;
public:
    InspectorMemoryAgent(PageAgentContext&);
    virtual ~InspectorMemoryAgent() { }

    virtual void didCreateFrontendAndBackend(Inspector::FrontendRouter*, Inspector::BackendDispatcher*) override;
    virtual void willDestroyFrontendAndBackend(Inspector::DisconnectReason) override;

    // MemoryBackendDispatcherHandler
    virtual void startTracking(ErrorString&) override;
    virtual void stopTracking(ErrorString&) override;

private:
    void collectSample(const ResourceUsageData&);

    std::unique_ptr<Inspector::MemoryFrontendDispatcher> m_frontendDispatcher;
    RefPtr<Inspector::MemoryBackendDispatcher> m_backendDispatcher;
    bool m_tracking { false };
};

} // namespace WebCore

#endif // ENABLE(RESOURCE_USAGE)

#endif // !defined(InspectorMemoryAgent_h)
