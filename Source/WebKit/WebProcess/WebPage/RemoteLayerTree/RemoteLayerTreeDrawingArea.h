/*
 * Copyright (C) 2012-2018 Apple Inc. All rights reserved.
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

#pragma once

#include "CallbackID.h"
#include "DrawingArea.h"
#include "GraphicsLayerCARemote.h"
#include "RemoteLayerTreeTransaction.h"
#include <WebCore/GraphicsLayerClient.h>
#include <WebCore/Timer.h>
#include <atomic>
#include <dispatch/dispatch.h>
#include <wtf/HashMap.h>
#include <wtf/WeakPtr.h>

namespace WebCore {
class PlatformCALayer;
}

namespace WebKit {

class RemoteLayerTreeContext;
class RemoteLayerTreeDisplayRefreshMonitor;

class RemoteLayerTreeDrawingArea : public DrawingArea, public CanMakeWeakPtr<RemoteLayerTreeDrawingArea>, public WebCore::GraphicsLayerClient {
    friend class RemoteLayerTreeDisplayRefreshMonitor;
public:
    RemoteLayerTreeDrawingArea(WebPage&, const WebPageCreationParameters&);
    virtual ~RemoteLayerTreeDrawingArea();

    TransactionID nextTransactionID() const { return m_currentTransactionID.next(); }
    TransactionID lastCommittedTransactionID() const { return m_currentTransactionID; }

private:
    // DrawingArea
    void setNeedsDisplay() override;
    void setNeedsDisplayInRect(const WebCore::IntRect&) override;
    void scroll(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollDelta) override;
    void updateGeometry(const WebCore::IntSize& viewSize, bool flushSynchronously, const WTF::MachSendRight& fencePort) override;
    void adoptDisplayRefreshMonitorsFromDrawingArea(DrawingArea&) override;

    WebCore::GraphicsLayerFactory* graphicsLayerFactory() override;
    void setRootCompositingLayer(WebCore::GraphicsLayer*) override;
    void scheduleInitialDeferredPaint() override;
    void scheduleCompositingLayerFlush() override;
    void scheduleCompositingLayerFlushImmediately() override;
    void attachViewOverlayGraphicsLayer(WebCore::GraphicsLayer*) override;

    void addTransactionCallbackID(CallbackID) override;

    RefPtr<WebCore::DisplayRefreshMonitor> createDisplayRefreshMonitor(WebCore::PlatformDisplayID) override;
    void willDestroyDisplayRefreshMonitor(WebCore::DisplayRefreshMonitor*);

    bool shouldUseTiledBackingForFrameView(const WebCore::FrameView&) const override;

    void updatePreferences(const WebPreferencesStore&) override;

    bool supportsAsyncScrolling() const override { return true; }
    bool usesDelegatedScrolling() const override { return true; }
    bool usesDelegatedPageScaling() const override { return true; }

    void setLayerTreeStateIsFrozen(bool) override;
    bool layerTreeStateIsFrozen() const override { return m_isFlushingSuspended; }
    bool layerFlushThrottlingIsActive() const override { return m_isThrottlingLayerFlushes && m_layerFlushTimer.isActive(); }

    void forceRepaint() override;
    bool forceRepaintAsync(CallbackID) override { return false; }

    void setViewExposedRect(Optional<WebCore::FloatRect>) override;
    Optional<WebCore::FloatRect> viewExposedRect() const override { return m_scrolledViewExposedRect; }

    void acceleratedAnimationDidStart(uint64_t layerID, const String& key, MonotonicTime startTime) override;
    void acceleratedAnimationDidEnd(uint64_t layerID, const String& key) override;

    WebCore::FloatRect exposedContentRect() const override;
    void setExposedContentRect(const WebCore::FloatRect&) override;

    void didUpdate() override;

#if PLATFORM(IOS_FAMILY)
    void setDeviceScaleFactor(float) override;
#endif

    void mainFrameContentSizeChanged(const WebCore::IntSize&) override;

    void activityStateDidChange(OptionSet<WebCore::ActivityState::Flag> changed, ActivityStateChangeID, const Vector<CallbackID>& callbackIDs) override;

    bool adjustLayerFlushThrottling(WebCore::LayerFlushThrottleState::Flags) override;

    bool addMilestonesToDispatch(OptionSet<WebCore::LayoutMilestone>) override;

    void updateScrolledExposedRect();
    void updateRootLayers();

    void addCommitHandlers();
    void flushLayers();

    WebCore::TiledBacking* mainFrameTiledBacking() const;

    TransactionID takeNextTransactionID() { return m_currentTransactionID.increment(); }

    bool markLayersVolatileImmediatelyIfPossible() override;

    void adoptLayersFromDrawingArea(DrawingArea&) override;

    class BackingStoreFlusher : public ThreadSafeRefCounted<BackingStoreFlusher> {
    public:
        static Ref<BackingStoreFlusher> create(IPC::Connection*, std::unique_ptr<IPC::Encoder>, Vector<RetainPtr<CGContextRef>>);

        void flush();
        bool hasFlushed() const { return m_hasFlushed; }

    private:
        BackingStoreFlusher(IPC::Connection*, std::unique_ptr<IPC::Encoder>, Vector<RetainPtr<CGContextRef>>);

        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_commitEncoder;
        Vector<RetainPtr<CGContextRef>> m_contextsToFlush;

        std::atomic<bool> m_hasFlushed;
    };

    std::unique_ptr<RemoteLayerTreeContext> m_remoteLayerTreeContext;
    Ref<WebCore::GraphicsLayer> m_rootLayer;

    WebCore::IntSize m_viewSize;

    Optional<WebCore::FloatRect> m_viewExposedRect;
    Optional<WebCore::FloatRect> m_scrolledViewExposedRect;

    WebCore::Timer m_layerFlushTimer;
    bool m_isFlushingSuspended { false };
    bool m_hasDeferredFlush { false };
    bool m_flushingInitialDeferredPaint { false };
    bool m_isThrottlingLayerFlushes { false };
    bool m_isLayerFlushThrottlingTemporarilyDisabledForInteraction { false };
    bool m_isInitialThrottledLayerFlush { false };

    bool m_waitingForBackingStoreSwap { false };
    bool m_hadFlushDeferredWhileWaitingForBackingStoreSwap { false };
    bool m_nextFlushIsForImmediatePaint { false };
    bool m_inFlushLayers { false };

    dispatch_queue_t m_commitQueue;
    RefPtr<BackingStoreFlusher> m_pendingBackingStoreFlusher;

    HashSet<RemoteLayerTreeDisplayRefreshMonitor*> m_displayRefreshMonitors;
    HashSet<RemoteLayerTreeDisplayRefreshMonitor*>* m_displayRefreshMonitorsToNotify { nullptr };

    TransactionID m_currentTransactionID;
    Vector<RemoteLayerTreeTransaction::TransactionCallbackID> m_pendingCallbackIDs;
    ActivityStateChangeID m_activityStateChangeID { ActivityStateChangeAsynchronous };

    OptionSet<WebCore::LayoutMilestone> m_pendingNewlyReachedPaintingMilestones;

    RefPtr<WebCore::GraphicsLayer> m_contentLayer;
    RefPtr<WebCore::GraphicsLayer> m_viewOverlayRootLayer;
};

inline bool RemoteLayerTreeDrawingArea::addMilestonesToDispatch(OptionSet<WebCore::LayoutMilestone> paintMilestones)
{
    m_pendingNewlyReachedPaintingMilestones.add(paintMilestones);
    return true;
}

} // namespace WebKit

SPECIALIZE_TYPE_TRAITS_DRAWING_AREA(RemoteLayerTreeDrawingArea, DrawingAreaTypeRemoteLayerTree)
