//----------------------------------------------------------------------------
//   $Id: zoombox.js,v 1.1.1.1 2007/04/24 02:04:22 rob Exp $
//----------------------------------------------------------------------------
// Find out if it is Netscape 5+
var NS4DOM = document.layers ? true:false;           // netscape 4
var IEDOM = document.all ? true:false;               // ie4+
var W3CDOM = document.getElementById && !(IEDOM) ? true:false;   // netscape 6
//var netscape = (document.layers) ? 1:0;
//var IEDOM = (document.all) ? 1:0;
//var netscape6 = (document.getElementById && !document.all) ? 1:0;
//alert ("Netscape" + netscape + " - IE " + goodIE + " - Netscape 6 " + netscape6);

// Constants
// IE6
//var vspcIEadjust = -7; var hspcIEadjust =  7;
var vspcIEadjust = 0; var hspcIEadjust =  0;
// Mozilla
var vspc = 21; var hspc = 16;

// Global vars to save mouse position
var mouseX = 0;
var mouseY = 0;
var x1 = 0;
var y1 = 0;
var x2 = 0;
var y2 = 0;
var firstx = 0;
var firsty = 0;
var secondx = 0;
var secondy = 0;
var zleft = 0;
var zright = 0;
var ztop = 0;
var zbottom = 0;
var mapX = 0;
var mapY = 0;
var dragging = false;

// Calculate the scale to draw a box on top of the image.
	// --- Use min/max x/y instead (rgp mods)
  // ---
  var extents = document.forms[0].extents.value.split(" ", document.forms[0].extents.value);
  var lft = extents[0];
  var bot = extents[1];
  var rht = extents[2];
  var top = extents[3];

  // --- Use min/max x/y instead (rgp mods)
  var rawminx = lft;
  var rawminy = bot;
  var rawmaxx = rht;
  var rawmaxy = top;
  // ---
  var scale = Math.abs(rht - lft)/iWidth;

// Caluculate the pixel ordinate.
  var lftp = Math.round((Math.abs(rawminx - lft))/scale);
  var botp = Math.round((Math.abs(rawminy - top))/scale);
  var rhtp = Math.round((Math.abs(rawmaxx - lft))/scale);
  var topp = Math.round((Math.abs(rawmaxy - top))/scale);

// Layers names
var toplayer = "zBoxTop";
var leftlayer = "zBoxLeft";
var bottomlayer = "zBoxBottom";
var rightlayer = "zBoxRight";

// Main script
setZoomBoxSettings();

var content;

//content = '<img name="img" src="[img]" border="1">';
//createLayer("mapimage", hspc, vspc, iWidth, iHeight, false, content);
content = '<img name="zImgTop" src="imagedir/pixel.gif" border="1">';
createLayer("zBoxTop", hspc, vspc, iWidth, iHeight, false, content);

content = '<img name="zImgLeft" src="imagedir/pixel.gif" border="1">';
createLayer("zBoxLeft", hspc, vspc, iWidth, iHeight, false, content);

content = '<img name="zImgBottom" src="imagedir/pixel.gif" border="1">';
createLayer("zBoxBottom", hspc, vspc, iWidth, iHeight, false, content);

content = '<img name="zImgRight" src="imagedir/pixel.gif" border="1">';
createLayer("zBoxRight", hspc, vspc, iWidth, iHeight, false, content);

// Search area box components.
var loverlay = "loverLay";
var roverlay = "roverLay";
var toverlay = "toverLay";
var boverlay = "boverLay";

// Search area box colour.
var overlayColour = "BLUE";

// Search area layers to draw the box.
content = '<img name="left" src="imagedir/pixel.gif" border="1">';
createLayer("loverLay", hspc, vspc, iWidth, iHeight, false, content);

content = '<img name="right" src="imagedir/pixel.gif" border="1">';
createLayer("roverLay", hspc, vspc, iWidth, iHeight, false, content);

content = '<img name="top" src="imagedir/pixel.gif" border="1">';
createLayer("toverLay", hspc, vspc, iWidth, iHeight, false, content);

content = '<img name="bottom" src="imagedir/pixel.gif" border="1">';
createLayer("boverLay", hspc, vspc, iWidth, iHeight, false, content);

// zoom box color
var boundingColor = "RED";
setLayerBackgroundColor("zBoxTop", boundingColor);
setLayerBackgroundColor("zBoxLeft", boundingColor);
setLayerBackgroundColor("zBoxRight", boundingColor);
setLayerBackgroundColor("zBoxBottom", boundingColor);

// Draw the blue search area box.
setLayerBackgroundColor("loverLay", overlayColour);
setLayerBackgroundColor("toverLay", overlayColour);
setLayerBackgroundColor("boverLay", overlayColour);
setLayerBackgroundColor("roverLay", overlayColour);

clipLayer(loverlay, lftp, topp, lftp + 2, botp);
clipLayer(toverlay, lftp, topp, rhtp, topp + 2);
clipLayer(boverlay, lftp, botp - 2, rhtp, botp);
clipLayer(roverlay, rhtp - 2, topp, rhtp, botp);

//showLayer(loverlay);
//showLayer(toverlay);
//showLayer(boverlay);
//showLayer(roverlay);


////////////////////////////
// Dynamic Map
/////////////////////////////
// Set global height and width
function setMapSize(iw, ih) {
    iWidth = iw;
    iHeight = ih;
}


// Create a DHTML layer
function createLayer(name, left, top, width, height, visible, content) {
  var layer, str;
  if (NS4DOM) {                        // Netscape
    str = '<layer id="' + name + '" left=' + left + ' top=' + top +
          ' width=' + width + ' height=' + height +
          ' visibility=' + (visible ? '"show"' : '"hide"') + '>';
    document.writeln(str);
    document.writeln(content);
    document.writeln('</layer>');
    layer = getLayer(name);
    layer.width = width;
    layer.height = height;
  } else if (IEDOM || W3CDOM) {                    // IE
    str = '<div id="' + name +
          '" style="position:absolute; overflow:none; left:' + left +
          'px; top:' + top + 'px; width:' + width + 'px; height:' + height +
          'px;' + ' visibility:' + (visible ? 'visible;' : 'hidden;') +  '">';
    document.writeln(str);
    document.writeln(content);
    document.writeln('</div>');
  } else {
    return null;
  }
  clipLayer(name, 0, 0, width, height);
}

// get the layer object called "name"
function getLayer(name) {
  if (NS4DOM) {                // Netscape
    return document.layers[name];
  } else if (IEDOM) {            // IE
    if (eval('document.all.' + name) != null) {
      layer = eval('document.all.' + name + '.style');
      return layer;
    } else {
      return null;
    }
  } else if (W3CDOM) {
    if (eval('document.getElementById("' + name + '")') != null) {
      layer = eval('document.getElementById("' + name + '").style');
      return layer;
    } else {
      return null;
    }
  } else {                              // Don't know
    return null;
  }
}

// set layer background color
function setLayerBackgroundColor(name, color) {
  var layer = getLayer(name);
  if (layer != null) {
    if (NS4DOM) {              // Netscape
      layer.bgColor = color;
    } else if (IEDOM || W3CDOM) {          // IE
      layer.backgroundColor = color;
    }
  }
}

// toggle layer to visible
function showLayer(name) {
  var layer = getLayer(name);
  if (layer != null) {
    if (NS4DOM) {              // Netscape
      layer.visibility = "show";
    } else if (IEDOM || W3CDOM) {          // IE
      layer.visibility = "visible";
    }
  }
}

// clip layer display to clipleft, cliptip, clipright, clipbottom
function clipLayer(name, clipleft, cliptop, clipright, clipbottom) {
    var layer = getLayer(name);
    if (layer != null) {
        if (isNaN(cliptop) ) cliptop = 0;
        if (isNaN(clipright) ) clipright = 0;
        if (isNaN(clipbottom) ) clipbottom = 0;
        if (isNaN(clipleft) ) clipleft = 0;
        if (NS4DOM) {                      // Netscape
            layer.clip.left   = clipleft;
            layer.clip.top    = cliptop;
            layer.clip.right  = clipright;
            layer.clip.bottom = clipbottom;
        }
        else if (IEDOM) {                  // IE
            layer.clip = 'rect(' + cliptop + ' ' +  clipright + ' ' +
                       clipbottom + ' ' + clipleft + ')';
        }
        else if (W3CDOM) {		// Netscape 6
            layer.clip = 'rect(' + cliptop + 'px ' + clipright + 'px ' +
                   clipbottom + 'px ' + clipleft + 'px)';
        }
    }
}

function setZoomBoxSettings() {
  // Set up event capture for mouse movement
  if (NS4DOM) {
    document.captureEvents(Event.MOUSEMOVE);
    document.captureEvents(Event.MOUSEDOWN);
    document.captureEvents(Event.MOUSEUP);
    document.captureEvents(Event.RESIZE);
  } else if (IEDOM) {
    hspc -= hspcIEadjust;
    vspc += vspcIEadjust;
  }
  document.onmousemove = getMouse;
  document.onmousedown = mapTool;
  document.onmouseup = chkMouseUp;
  window.onresize = handleresize;
}

function handleresize() {
  if (NS4DOM || W3CDOM) {		// Netscape
    location.reload();
  }
}

// check for mouseup
function chkMouseUp(e) {
  if (dragging) {
    mouseX = Math.min(Math.max(mouseX, 0), iWidth);
    mouseY = Math.min(Math.max(mouseY, 0), iHeight);
    mapTool(e);
  }
}

// Mouse down / drag
function mapTool(e) {
//alert ("dragging=" + dragging + ", type=" + e.target.type);
  if (W3CDOM && !dragging && e.target.type != "image") return true;
  getImageXY(e);
  if (!dragging && insideMap()) {
    startZoomBox(e);
    return false;
  } else if (dragging) {
    getMouse(e);
    stopZoomBox(e);
  }
  return true;
}

function getImageXY(e) {
  if (NS4DOM || W3CDOM) {                // Netscape
    mouseX = e.pageX;
    mouseY = e.pageY;
  } else if (IEDOM) {            // IE
    //mouseX = event.clientX + document.body.scrollLeft;
    //mouseY = event.clientY + document.body.scrollTop;
    mouseX = event.clientX + document.documentElement.scrollLeft;  //Strict DTD ?
    mouseY = event.clientY + document.documentElement.scrollTop;
  } else {                              // Don't know
    mouseX = mouseY = 0;
  }
  // subtract offsets from page left and top
  mouseX = mouseX - hspc;
  mouseY = mouseY - vspc;
}

// Mouse move
// get the coords at mouse position
function getMouse(e) {
  window.status = "";
  getImageXY(e);

  if (dragging) {
    x2 = mouseX = Math.min(Math.max(mouseX, 0), iWidth);
    y2 = mouseY = Math.min(Math.max(mouseY, 0), iHeight);
    setClip();
    return false;
  } else {
    return true;
  }
  return true;
}

function insideMap() {
  return ((mouseX >= 0) && (mouseX < iWidth) &&
(mouseY >= 0) && (mouseY < iHeight));
}

// start zoom in.... box displayed
function startZoomBox(e) {
  getImageXY(e);
  // keep it within the MapImage
  if (!dragging) {
    // capture values to pass to map server
    firstx = x1 = mouseX;
    firsty = y1 = mouseY;
    x2 = x1 + 1;
    y2 = y1 + 1;

    clipLayer(toplayer, x1, y1, x2, y2);
    clipLayer(leftlayer, x1, y1, x2, y2);
    clipLayer(rightlayer, x1, y1, x2, y2);
    clipLayer(bottomlayer, x1, y1, x2, y2);
    showLayer(toplayer);
    showLayer(leftlayer);
    showLayer(rightlayer);
    showLayer(bottomlayer);
    dragging = true;
  } else {
    stopZoomBox(e);
  }
  return false;
}

// stop zoom box display... zoom in
function stopZoomBox(e) {
  dragging = false;

  secondx = x2;
  secondy = y2;

  //document.forms[0].mainmap.value = "";
  if (firstx == secondx && firsty == secondy) {
    // If click on 1 point, change mapaction to zoom or pan.
    document.forms[0].imgxy.value = firstx + " " + firsty;
    document.forms[0].imgbox.value = "";

    submitForm();   // Set hidden form variables
    document.forms[0].submit();
  } else {
    // Zoom to box
    var tx1 = Math.min(firstx, secondx);
    var tx2 = Math.max(firstx, secondx);
    var ty1 = Math.min(firsty, secondy);
    var ty2 = Math.max(firsty, secondy);
    // Don't know why these were set this way
    //iWidth = tx2 - tx1;
    //iHeight = ty2 - ty1;
    document.forms[0].imgbox.value = tx1 + " " + ty1 + " " + tx2 + " " + ty2;
    submitForm();
    document.forms[0].submit();
  }
  return false;
}

// clip zoom box layer to mouse coords
function setClip() {
  zright  = Math.max(x1, x2);
  zleft   = Math.min(x1, x2);
  zbottom = Math.max(y1, y2);
  ztop    = Math.min(y1, y2);

  if ((x1 != x2) && (y1 != y2)) {
    var ovBoxSize = 1;
    clipLayer(toplayer, zleft, ztop, zright, ztop + ovBoxSize);
    clipLayer(leftlayer, zleft, ztop, zleft + ovBoxSize, zbottom);
    clipLayer(rightlayer, zright - ovBoxSize, ztop, zright, zbottom);
    clipLayer(bottomlayer, zleft, zbottom - ovBoxSize, zright, zbottom);
  }
}


