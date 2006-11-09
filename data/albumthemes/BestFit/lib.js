/*************************************************
* lib.js
* Copyright (C) 2005-2006 Free Software Foundation, Inc.
* $Id$
*
* Scripts for DHTML photo album template
* Various library routines used by other scripts
*************************************************/

// deleted an object from within its parent
function deleteObject ( parentObj, obj )
{
    parentObj.removeChild(obj);
}

// returns all elements of a certain type (tag) and class
function getElementsByClass ( tag, classname )
{
    var objs = new Array();
    var all = document.getElementsByTagName(tag);
    for (var i=0; i<all.length; i++)
    {
        if (all[i].className == classname)
            objs[objs.length] = all[i];
    }
    return objs;
}

// set a property to a given value on an object
function setProperty ( obj, prop, val )
{
    if (obj.style.setProperty) // W3C
        obj.style.setProperty(prop, val, null);
    else // MSIE
        eval("obj.style." + prop + " = \"" + val + "\"");
}
 
// gets a style property for an object
function getProperty ( obj, prop )
{
    if (document.defaultView && document.defaultView.getComputedStyle)
    {
        var val = document.defaultView.getComputedStyle(obj,null).getPropertyValue(prop)
        if (val)
            return val;
        else
            return eval("document.defaultView.getComputedStyle(obj,null)." + prop);
    }
    else if (window.getComputedStyle) // Konqueror
        return window.getComputedStyle(obj,null).getPropertyValue(prop);
    else if (obj.currentStyle) // MSIE
        return eval('obj.currentStyle.' + prop);
}

function getChildren ( obj )
{
    if (obj.children)
        return obj.children;
    
    var children = new Array();
    for (var i=0; i<obj.childNodes.length; i++)
    {
        if (obj.childNodes[i].nodeName.charAt(0) != '#')
            children[children.length] = obj.childNodes[i];
    }
    
    return children;
}

function getPropertyPx ( obj, prop )
{
    var w = getProperty(obj, prop);

    // find the begining of the number, after a space
    var a = w.indexOf(" ");
    if (a == -1)
        a = 0;
    // find the end of the number, before "px"
    var b = w.indexOf("px");
    
    if (b != -1)
        return parseInt(w.substr(a, b-a));
    else
        return 0;
}
        
// gets the offset from the top of the screen to the top of an object
function getTopOffset ( obj )
{
    var offset = getProperty(obj, "top");
    
    // find the beginning of the number, after a space
    var a = offset.indexOf(" ");
    if (a == -1)
        a = 0;
    // find the end of the number, before "px"
    var b = offset.indexOf("px");
    
    if (b != -1)
        return parseInt(offset.substr(a, b-a));
    else // value not returned in pixels
        return obj.offsetTop;
}

// gets the offset from the left of the screen to the left of an object
function getLeftOffset ( obj )
{
    var offset = getProperty(obj, "left");
    
    // find the beginning of the number, after a space
    var a = offset.indexOf(" ");
    if (a == -1)
        a = 0;
    // find the end of the number, before "px"
    var b = offset.indexOf("px");
    
    if (b != -1)
        return parseInt(offset.substr(a, b-a));
    else // value not returned in pixels
        return obj.offsetLeft;
}

// gets the height of the current window, in pixels
function getWindowHeight ()
{
    if (document.defaultView && document.defaultView.innerHeight)
        return document.defaultView.innerHeight;
    else if (window.innerHeight) // Konqueror
        return window.innerHeight;
    else if (document.body.offsetHeight) // MSIE
        return document.body.offsetHeight;
}

// gets the width of the current window, in pixels
function getWindowWidth ()
{
    if (document.defaultView && document.defaultView.innerWidth)
        return document.defaultView.innerWidth;
    else if (window.innerWidth) // Konqueror
        return window.innerWidth;
    else if (document.body.clientWidth) // MSIE
        return document.body.clientWidth;
}

// gets the width of ab object, in pixels
function getObjWidth ( obj )
{
    var w = getProperty(obj, "width");

    // find the begining of the number, after a space
    var a = w.indexOf(" ");
    if (a == -1)
        a = 0;
    // find the end of the number, before "px"
    var b = w.indexOf("px");
    
    if (b != -1)
        return parseInt(w.substr(a, b-a));
    else // value not returned in pixels
        return obj.clientWidth;
}
        
// gets the height of an object, in pixels
function getObjHeight ( obj )
{
    var w = getProperty(obj, "height");

    // find the begining of the number, after a space
    var a = w.indexOf(" ");
    if (a == -1)
        a = 0;
    // find the end of the number, before "px"
    var b = w.indexOf("px");
    
    if (b != -1)
        return parseInt(w.substr(a, b-a));
    else // value not returned in pixels
        return obj.clientHeight;
}
        
// gets the total width of all horizontal borders and padding of an object
function getHBorder ( obj )
{
    // this won't work if the units aren't pixels
    var leftBorder = parseInt(getProperty(obj, "borderLeftWidth").replace(/[^0-9\.]/gi, ""));
    var leftPad = parseInt(getProperty(obj, "paddingLeft").replace(/[^0-9\.]/gi, ""));
    var rightPad = parseInt(getProperty(obj, "paddingRight").replace(/[^0-9\.]/gi, ""));
    var rightBorder = parseInt(getProperty(obj, "borderRightWidth").replace(/[^0-9\.]/gi, ""));
            
    // assign a sane value if the unit wasn't in pixels
    if (isNaN(leftBorder))
        leftBorder = 0;
    if (isNaN(leftPad))
        leftPad = 0;
    if (isNaN(rightPad))
        rightPad = 0;
    if (isNaN(rightBorder))
        rightBorder = 0;
    
    return leftBorder + leftPad + rightPad + rightBorder;
}
        
// gets the total width of all vertical borders and padding of an object
function getVBorder ( obj )
{
    // this won't work if the units aren't pixels
    var topBorder = parseInt(getProperty(obj, "borderTopWidth").replace(/[^0-9\.]/gi, ""));
    var topPad = parseInt(getProperty(obj, "paddingTop").replace(/[^0-9\.]/gi, ""));
    var bottomPad = parseInt(getProperty(obj, "paddingBottom").replace(/[^0-9\.]/gi, ""));
    var bottomBorder = parseInt(getProperty(obj, "borderBottomWidth").replace(/[^0-9\.]/gi, ""));

    // assign a sane value if the unit wasn't in pixels
    if (isNaN(topBorder))
        topBorder = 0;
    if (isNaN(topPad))
        topPad = 0;
    if (isNaN(bottomPad))
        bottomPad = 0;
    if (isNaN(bottomBorder))
        bottomBorder = 0;
    
    return topBorder + topPad + bottomPad + bottomBorder;
}

function getTotalWidth ( obj )
{
    var wid = 0;
    if (obj.offsetWidth)
        wid = obj.offsetWidth;
    else
    {
        // need to add object, padding, and border widths
        wid += getPropertyPx(obj, "border-left-width");
        wid += getPropertyPx(obj, "padding-left");
        wid += getPropertyPx(obj, "width");
        wid += getPropertyPx(obj, "padding-right");
        wid += getPropertyPx(obj, "border-right-width");
    }
    
    // add margin widths
    wid += getPropertyPx(obj, "margin-left");
    wid += getPropertyPx(obj, "margin-right");
    
    return wid;
}

