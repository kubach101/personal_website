
function dragElement(element) {
    var initialX = 0;
    var initialY = 0;
    var currentX = 0;
    var currentY = 0;

    if (document.getElementById(element.id + "-header")) {
        document.getElementById(element.id + "-header").onmousedown = startDragging;
    } else {

        element.onmousedown = startDragging;
    }

    function startDragging(e) {
        e = e || window.event;
        e.preventDefault();
        initialX = e.clientX;
        initialY = e.clientY;
        document.onmouseup = stopDragging;
        document.onmousemove = dragElement;
    }

    function dragElement(e) {
        e = e || window.event;
        e.preventDefault();
        currentX = initialX - e.clientX;
        currentY = initialY - e.clientY;
        initialX = e.clientX;
        initialY = e.clientY;
        element.style.top = (element.offsetTop - currentY) + "px";
        element.style.left = (element.offsetLeft - currentX) + "px";
    }

    function stopDragging() {
        document.onmouseup = null;
        document.onmousemove = null;
    }
}
function closeWindow(element) {
    element.style.display = "none"
}

var topIdx = 1;
var topBar = document.querySelector("#top");

function openWindow(element) {
    element.style.display = "flex";
    topIdx++;
    element.style.zIndex = topIdx;
    topBar.style.zIndex = topIdx + 1;

}

function handleWindowTap(element) {
    topIdx++;
    element.style.zIndex = topIdx;
    topBar.style.zIndex = topIdx + 1;

}

var selectedIcon = undefined

function handleIconTap(icon, window) {
    if (icon.classList.contains("selected")) {
        icon.classList.remove("selected");
        selectedIcon = undefined;
        openWindow(window);
    } else {
        if (selectedIcon) {
            selectedIcon.classList.remove("selected");
        }
        icon.classList.add("selected");
        selectedIcon = icon;
    }
}

function windowInit(appName) {
    var appWin = document.querySelector("#" + appName + "-win");
    var appWinClose = document.querySelector("#" + appName + "-close-btn");
    var appIcon = document.querySelector("#" + appName + "-btn");

    dragElement(appWin)
    appIcon.addEventListener("click", () => handleIconTap(appIcon, appWin));
    appWinClose.addEventListener("click", () => closeWindow(appWin));
    appWin.addEventListener("mousedown", () => handleWindowTap(appWin));
}

windowInit("calc")
windowInit("note")
windowInit("timer")
windowInit("settings")
