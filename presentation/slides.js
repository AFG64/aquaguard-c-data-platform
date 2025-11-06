(function() {
  const slides = Array.from(document.querySelectorAll('.slide'));
  let idx = 0;
  function show(i) {
    idx = (i + slides.length) % slides.length;
    slides.forEach((s, j) => s.style.display = j === idx ? 'grid' : 'none');
  }
  show(0);
  window.addEventListener('keydown', (e) => {
    if (['ArrowRight','PageDown',' '].includes(e.key)) show(idx+1);
    if (['ArrowLeft','PageUp','Backspace'].includes(e.key)) show(idx-1);
  });
})();
