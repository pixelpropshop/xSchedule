$(document).ready(function() {
  //populate current page
  updateCurrentPage(true);

});

window.addEventListener('popstate', function(event) {
  updateCurrentPage(true);
})
var currentPage;

function updateCurrentPage(fromHistory) {
  var currentPageArgs = getQueryVariable("args");
  if (currentPageArgs == false) {
    currentPageArgs = undefined;
  }
  if (getQueryVariable("page") != false) {
    updatePage('page', getQueryVariable("page"), currentPageArgs, fromHistory);
  } else if (getQueryVariable("plugin") != false) {
    updatePage('plugin', getQueryVariable("plugin"), currentPageArgs, fromHistory);
  } else {
    updatePage('page', 'home', undefined, fromHistory);
  }
}

// fromHistory: the URL already shows this page (first load, back/forward), so no new history entry is added
function updatePage(type, pageName, args, fromHistory) {
  currentPage = pageName;
  $('.xs-tabs li, .xs-bottom a').removeClass('active');
  $('.dropdown.open').removeClass('open');

  var url;
  if (type == "page") {
    $('#pageContent').load('pages/' + pageName + '.html', function() {
      xsFillIcons('#pageContent');
    });
    $('[data-page="' + pageName + '"]').addClass('active');
    url = 'index.html?page=' + pageName;
  } else if (type == "plugin") {
    var folder = encodeURI(pageName);
    $('#pageContent').load('Plugins/' + folder + '/' + folder + '.html');
    $('[data-page="plugins"]').addClass('active');
    url = 'index.html?plugin=' + folder;
  }
  if (args != undefined && args !== "") {
    url += '&args=' + encodeURIComponent(args);
  }
  if (!fromHistory && url) {
    window.history.pushState('page2', pageName, url);
  }
  window.scrollTo(0, 0);
}

function getQueryVariable(variable) {
  var query = window.location.search.substring(1);
  var vars = query.split("&");
  for (var i = 0; i < vars.length; i++) {
    var pair = vars[i].split("=");
    if (pair[0] == variable) {
      return decodeURIComponent(pair[1]);
    }
  }
  return (false);
}
