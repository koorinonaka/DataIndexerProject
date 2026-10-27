// トップのコード枠: C++ / Blueprint のタブと copy ボタン
document.addEventListener('DOMContentLoaded', function () {
  var tabs = document.querySelectorAll('.di-codetab');
  if (!tabs.length) return;
  var copy = document.querySelector('.di-codebox__copy');

  tabs.forEach(function (btn) {
    btn.addEventListener('click', function () {
      var target = btn.dataset.tab;
      tabs.forEach(function (b) {
        var on = b === btn;
        b.setAttribute('aria-selected', on ? 'true' : 'false');
        b.tabIndex = on ? 0 : -1;
      });
      document.querySelectorAll('.di-codepane').forEach(function (p) {
        p.hidden = p.dataset.pane !== target;
      });
      // copy はコードのタブでだけ出す (Blueprint は画像なので写すものが無い)
      if (copy) copy.hidden = target !== 'cpp';
    });
  });

  if (copy) {
    copy.addEventListener('click', function () {
      var code = document.getElementById(copy.dataset.copy);
      if (!code || !navigator.clipboard) return;
      navigator.clipboard.writeText(code.textContent).then(function () {
        var label = copy.querySelector('span');
        label.textContent = 'copied';
        setTimeout(function () { label.textContent = 'copy'; }, 1500);
      });
    });
  }
});
