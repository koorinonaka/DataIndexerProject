// TOC active state fix for sticky header + tabs offset
// Material's IntersectionObserver threshold = header height only (52px),
// but tabs add 42px more. Heading lands at 94px (scroll-margin-top) after
// anchor click — above our 96px threshold but below Material's, so the
// previous item stays active. Also handles bottom-of-page edge case.
;(function () {
  function getOffset() {
    var h = document.querySelector('.md-header');
    var t = document.querySelector('.md-tabs');
    return (h ? h.offsetHeight : 52) + (t ? t.offsetHeight : 42) + 2;
  }

  function isNearBottom() {
    return window.scrollY + window.innerHeight >= document.documentElement.scrollHeight - 20;
  }

  function syncActive() {
    var offset = getOffset();
    var atBottom = isNearBottom();
    var headings = Array.from(
      document.querySelectorAll('.md-typeset :is(h2,h3,h4,h5,h6)[id]')
    );
    if (!headings.length) return;

    var activeId = null;
    for (var i = headings.length - 1; i >= 0; i--) {
      var top = headings[i].getBoundingClientRect().top;
      // At bottom of page use full viewport height; otherwise use sticky offset
      if (atBottom ? top < window.innerHeight : top <= offset) {
        activeId = headings[i].id;
        break;
      }
    }
    if (!activeId) return;

    var hash = '#' + activeId;
    document.querySelectorAll('.md-sidebar--primary .md-nav__link').forEach(function (link) {
      var href = link.getAttribute('href') || '';
      if (!href.startsWith('#') && !href.includes('/#')) return;
      link.classList.toggle('md-nav__link--active', href === hash || href.endsWith(hash));
    });
  }

  // Wait for Material's IO to fire first (~1 frame), then correct
  window.addEventListener('hashchange', function () {
    setTimeout(syncActive, 60);
  });

  if (window.location.hash) {
    setTimeout(syncActive, 60);
  }
})();
