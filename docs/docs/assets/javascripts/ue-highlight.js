// 本文のコードブロック: Pygments は UE のマクロも型名も普通の名前 (.n) で出すので、ここで区別する
(function () {
  var MACRO = /^[A-Z][A-Z0-9_]+$/;          // UCLASS(, DI_DEFINE_INDEX(
  var TYPE = /^[FUAETI][A-Z][A-Za-z0-9]*[a-z][A-Za-z0-9]*$/; // FItemRow, UItemSchema, TObjectPtr (小文字を含む: UI は除く)

  function mark() {
    document.querySelectorAll('.language-cpp.highlight span.n').forEach(function (el) {
      var name = el.textContent;
      var next = el.nextElementSibling;
      if (MACRO.test(name) && next && next.textContent.charAt(0) === '(') {
        el.classList.add('ue-macro');
      } else if (TYPE.test(name)) {
        el.classList.add('ue-type');
      }
    });
  }

  // document$ は Material が出す。instant navigation を有効にしても遷移ごとに走る
  if (window.document$) document$.subscribe(mark);
  else document.addEventListener('DOMContentLoaded', mark);
})();
