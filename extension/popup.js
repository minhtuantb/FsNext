// SPDX-License-Identifier: Proprietary
const btn    = document.getElementById('send');
const status = document.getElementById('status');

btn.addEventListener('click', () => {
  btn.disabled = true;
  status.textContent = 'Đang gửi…';
  chrome.runtime.sendMessage({ type: 'send-current' }, (resp) => {
    btn.disabled = false;
    if (resp && resp.ok) {
      status.textContent = 'Đã gửi.';
      setTimeout(() => window.close(), 800);
    } else {
      status.textContent = 'Không gửi được — kiểm tra Fshare Tool đã chạy chưa.';
    }
  });
});
