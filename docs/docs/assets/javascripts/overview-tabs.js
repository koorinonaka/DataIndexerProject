document.addEventListener('DOMContentLoaded', function () {
  var tabs = document.querySelectorAll('.ov-dual-tab');
  if (!tabs.length) return;
  tabs.forEach(function (btn) {
    btn.addEventListener('click', function () {
      var target = btn.dataset.tab;
      document.querySelectorAll('.ov-dual-tab').forEach(function (b) {
        b.classList.toggle('active', b === btn);
      });
      document.querySelectorAll('.ov-dual-pane').forEach(function (p) {
        p.hidden = p.dataset.pane !== target;
      });
    });
  });
});
