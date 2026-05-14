// SPDX-License-Identifier: Proprietary
// Background service worker — registers a context-menu entry for Fshare URLs
// and routes user-clicked links to the native host.

const HOST = "com.fshare.tool";

function isFshareUrl(url) {
  try {
    const u = new URL(url);
    return /(^|\.)fshare\.vn$/i.test(u.hostname);
  } catch { return false; }
}

function send(url) {
  if (!isFshareUrl(url)) return;
  chrome.runtime.sendNativeMessage(HOST, { url }, (resp) => {
    if (chrome.runtime.lastError) {
      // Native host missing or registration broken — surface a one-shot
      // notification instead of silently dropping the click.
      console.warn("Fshare native host error:", chrome.runtime.lastError.message);
      return;
    }
    if (resp && resp.ok) {
      chrome.action.setBadgeText({ text: "✓" });
      setTimeout(() => chrome.action.setBadgeText({ text: "" }), 1200);
    } else {
      chrome.action.setBadgeText({ text: "!" });
    }
  });
}

chrome.runtime.onInstalled.addListener(() => {
  chrome.contextMenus.create({
    id: "send-to-fshare",
    title: "Gửi link đến Fshare Tool",
    contexts: ["link", "page"],
    targetUrlPatterns: ["*://*.fshare.vn/*"],
    documentUrlPatterns: ["*://*.fshare.vn/*", "*://*/*"]
  });
});

chrome.contextMenus.onClicked.addListener((info, tab) => {
  if (info.menuItemId !== "send-to-fshare") return;
  const url = info.linkUrl || info.pageUrl || (tab && tab.url);
  send(url);
});

// Popup → background bridge.
chrome.runtime.onMessage.addListener((msg, _sender, sendResponse) => {
  if (msg && msg.type === "send-current") {
    chrome.tabs.query({ active: true, currentWindow: true }, ([tab]) => {
      if (tab && tab.url) {
        send(tab.url);
        sendResponse({ ok: true });
      } else {
        sendResponse({ ok: false, error: "no-tab" });
      }
    });
    return true; // keep the channel open for async response
  }
});
