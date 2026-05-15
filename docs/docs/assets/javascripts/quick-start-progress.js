document.addEventListener('DOMContentLoaded', function () {
  var rail = document.querySelector('.qs-progress');
  if (!rail) return;

  var stepHeadings = Array.from(document.querySelectorAll('h2[id^="step-"]'));
  var completeCards = Array.from(document.querySelectorAll('.qs-complete'));
  var dots = Array.from(document.querySelectorAll('.qs-progress-step'));

  if (!stepHeadings.length || !dots.length) return;

  // Build trigger list: step headings (→ current) + completion cards (→ done)
  var triggers = [];
  stepHeadings.forEach(function (h, i) {
    triggers.push({ el: h, step: i + 1, done: false });
  });
  completeCards.forEach(function (c, i) {
    if (i < stepHeadings.length) {
      triggers.push({ el: c, step: i + 1, done: true });
    }
  });
  // Sort by DOM order
  triggers.sort(function (a, b) {
    return (a.el.compareDocumentPosition(b.el) & Node.DOCUMENT_POSITION_FOLLOWING) ? -1 : 1;
  });

  var totalSteps = stepHeadings.length; // 3

  function activate(step, done) {
    var allDone = done && step >= totalSteps;
    dots.forEach(function (dot, i) {
      dot.classList.remove('current', 'done');
      // steps before current: done
      // current step when done: done
      // finish dot when all done: done
      if (i + 1 < step || (done && i + 1 === step) || (allDone && i + 1 > step)) {
        dot.classList.add('done');
      } else if (!done && i + 1 === step) {
        dot.classList.add('current');
      }
    });
    var doneCount = done ? step : step - 1;
    var fillPct = doneCount <= 0 ? '0%' : Math.round(doneCount / totalSteps * 100) + '%';
    rail.style.setProperty('--qs-progress-fill', fillPct);
  }

  activate(1, false);

  var ticking = false;
  function onScroll() {
    if (ticking) return;
    ticking = true;
    requestAnimationFrame(function () {
      // At the very bottom of the page, force all triggers to fire
      var scrolled = window.scrollY + window.innerHeight;
      var pageH = document.documentElement.scrollHeight;
      var triggerY = (scrolled >= pageH - 100)
        ? pageH
        : window.scrollY + window.innerHeight * 0.6;

      var curStep = 1, curDone = false;
      triggers.forEach(function (t) {
        if (t.el.getBoundingClientRect().top + window.scrollY <= triggerY) {
          curStep = t.step;
          curDone = t.done;
        }
      });
      activate(curStep, curDone);
      ticking = false;
    });
  }

  window.addEventListener('scroll', onScroll, { passive: true });
  onScroll();
});
