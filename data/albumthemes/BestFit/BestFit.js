/*************************************************
* BestFit.js
* Copyright (c) Rennie deGraaf, 2005.  All rights reserved.
* $Id$
*
* Scripts for DHTML photo album template
* The script in this file runs every time the page is reloaded or resized, and
* does the following:
*  - corrects CSS rendering bugs found in Opera, Konqueror, and MSIE
*  - resizes the image preview to the largest size that will fit in its 
*    container, up to the actual size of the image, without losing aspect ratio
*  - vertically centers several text blocks within the current dimensions of 
*    their containers
*
* NOTE: if you change the layout in the stylesheet, you'll likely also have to 
* change the correct() function in this script
*************************************************/

// globals
var imageWidth;     // the base width of image, in pixels
var imageHeight;    // the base height of image, in pixels

// gets the first child of an element
function getFirstChild ( obj )
{
    if (obj.children)
    {
        if (obj.children.length == 0)
            return null;
        return obj.children[0];
    }
    else
    {
        // childNodes contains all sorts of #text and #comment elements...
        var i=0;
        while (obj.childNodes[i].nodeName.charAt(0) == '#')
        {
            i++;
        }
    
        if (i == 0)
            return null;
        return obj.childNodes[i];
    }
}
        
// resizes an image to the maximum size that will fit in its container, 
// while preserving aspect ratio
function resize ()
{
    var image;          // the image to be modified
    var imageAspect;    // the width/height aspect ratio of image
    var imagePanel;     // the panel to hold the image
    var notePanel;      // the panel to hold the note
    var noteHeight;     // the height of the note panel
    var imageBorder;    // the width of the border on image, in pixels
    var panelWidth;     // the current width of imagePanel, in pixels
    var panelHeight;    // the current height of imagePanel, in pixels
    var panelAspect;    // the current aspect ratio of imagePanel
    
    image = document.getElementById("image");
    imageAspect = imageWidth/imageHeight;
    imagePanel = document.getElementById("imagePanel");

    notePanel = document.getElementById("fullsizenote");
    noteHeight = 0;
    if (notePanel != null)
        noteHeight = getObjHeight(document.getElementById("fullsizenote"));
            
    imageBorder = getVBorder(image);
    panelWidth = getObjWidth(imagePanel) - getHBorder(image);
    panelHeight = getObjHeight(imagePanel) - noteHeight - getVBorder(image);
    panelAspect = panelWidth/panelHeight;

    // set the width and height of the image
    if (imageAspect >= panelAspect)
    {
        // constrained by width
        image.width = Math.min(panelWidth, imageWidth);
        image.height = Math.min((imageHeight*panelWidth/imageWidth), imageHeight);
    }
    else
    {
        // constrained by height
        image.height = Math.min(panelHeight, imageHeight);
        image.width = Math.min((imageWidth*panelHeight/imageHeight), imageWidth);
    }
}

// corrects various browser rendering bugs
function correct ()
{
    // correct center column width in Opera and MSIE
    var centerWidth = getObjWidth(document.getElementById("imagePanel"));
    var rightWidth = getObjWidth(document.getElementById("propertyPanel"));
    var leftWidth = getObjWidth(document.getElementById("captionPanel"));
    var bodyWidth = getWindowWidth();
    var diff = bodyWidth - (centerWidth + rightWidth + leftWidth);
    if (diff > 2 || diff < -2)
    {
        setProperty(document.getElementById("imagePanel"), "width", (bodyWidth - (rightWidth + leftWidth))+"px");
    }
    var titleWidth = getObjWidth(document.getElementById("titlePanel"));
    var diff = bodyWidth - (titleWidth + rightWidth + leftWidth);
    if (diff > 2 || diff < -2)
    {
        setProperty(document.getElementById("titlePanel"), "width", (bodyWidth - (rightWidth + leftWidth))+"px");
    }
    var copyrightWidth = getObjWidth(document.getElementById("copyrightPanel"));
    var diff = bodyWidth - (copyrightWidth + rightWidth + leftWidth);
    if (diff > 2 || diff < -2)
    {
        setProperty(document.getElementById("copyrightPanel"), "width", (bodyWidth - (rightWidth + leftWidth))+"px");
    }

    // correct center column height in Konqueror
    var centerHeight = getObjHeight(document.getElementById("imagePanel"));
    var titleHeight = getObjHeight(document.getElementById("titlePanel"));
    var copyrightHeight = getObjHeight(document.getElementById("copyrightPanel"));
    var bodyHeight = getWindowHeight();
    diff = bodyHeight - (centerHeight + titleHeight + copyrightHeight);
    if (diff > 2 || diff < -2)
    {
        setProperty(document.getElementById("imagePanel"), "height", (bodyHeight - (titleHeight + copyrightHeight))+"px");
    }

    // correct side panel heights in MSIE
    var captionHeight = getObjHeight(document.getElementById("captionPanel"));
    var propertyHeight = getObjHeight(document.getElementById("propertyPanel"));
    var captionOffset = getTopOffset(document.getElementById("captionPanel"));
    var propertyOffset = getTopOffset(document.getElementById("propertyPanel"));
    var captionBorderHeight = getVBorder(document.getElementById("captionPanel"));
    var propertyBorderHeight = getVBorder(document.getElementById("propertyPanel"));
    diff = bodyHeight - captionHeight - captionBorderHeight - captionOffset;
    if (diff != 0)
    {
        setProperty(document.getElementById("captionPanel"), "height", (bodyHeight-captionBorderHeight-captionOffset-2)+"px");
    }
    diff = bodyHeight - propertyHeight - propertyBorderHeight - propertyOffset;
    if (diff != 0)
    {
        setProperty(document.getElementById("propertyPanel"), "height", (bodyHeight-propertyBorderHeight-propertyOffset-2)+"px");
    }
}

// centers all DIVs of class "centerContent"
function center ()
{
    // figure out what the variable for "margin-left" is
    var left=0;
    var marginLeft = "margin-left";
    if (document.body.currentStyle && isNaN(document.body.currentStyle.margin-left))
        marginLeft = "marginLeft";

    // locate all DIVs of class "centerContent"
    var objs = getElementsByClass("div", "centerContent");
    var objHeight;
    var objWidth;
    for (var i=0; i<objs.length; i++)
    {
        // set their widths and heights according to the widths and heights of
        // their contents
        // NOTE: this will not work properly if a "centerContent" DIV contains 
        // more than one element
        objHeight = getObjHeight(getFirstChild(objs[i]));
        objWidth = getObjWidth(getFirstChild(objs[i]));
        if (objHeight != 0) // Opera doesn't have dimensions of inline objects
        {
            setProperty(objs[i], "height", (objHeight)+"px");
            setProperty(objs[i], "top", (objHeight/(-2.0))+"px");
        }
        if (objWidth != 0)
        {
            setProperty(objs[i], "width", (objWidth)+"px");
            setProperty(objs[i], marginLeft, (objWidth/(-2.0))+"px");
        }
    }
}
     
function resizeAction ()
{
    // set initial image size to 1x1 so that the real size doesn't affect layout
    document.getElementById("image").width = 1;
    document.getElementById("image").height = 1;

    // figure out what the variable for "margin-left" is
    var left=0;
    var marginLeft = "margin-left";
    if (document.body.currentStyle && isNaN(document.body.currentStyle.margin-left))
        marginLeft = "marginLeft";
    
    // reset all values that we play with, so that the browser can
    // figure layout on its own as much as possible
    setProperty(document.getElementById("imagePanel"), "width", "");
    setProperty(document.getElementById("titlePanel"), "width", "");
    setProperty(document.getElementById("copyrightPanel"), "width", "");
    setProperty(document.getElementById("imagePanel"), "height", "");
    setProperty(document.getElementById("captionPanel"), "height", "");
    setProperty(document.getElementById("propertyPanel"), "height", "");
    if (getObjHeight(document.getElementById("titleContent")) != 0)
    {
        setProperty(document.getElementById("titleContent").parentNode, "height", "");
        setProperty(document.getElementById("titleContent").parentNode, "top", "");
        setProperty(document.getElementById("titleContent").parentNode, "width", "");
        setProperty(document.getElementById("titleContent").parentNode, marginLeft, "");
    }
    if (getObjHeight(document.getElementById("copyrightContent")) != 0)
    {
        setProperty(document.getElementById("copyrightContent").parentNode, "height", "");
        setProperty(document.getElementById("copyrightContent").parentNode, "top", "");
        setProperty(document.getElementById("copyrightContent").parentNode, "width", "");
        setProperty(document.getElementById("copyrightContent").parentNode, marginLeft, "");
    }

    // make corrections to work around browser bugs
    correct();
    
    // sleep briefly so that Konqueror gets properties correctly, then
    // resize the image according to screen size
    setTimeout("resize(); center()", 1);
}

// performs all initialization and calls the actual action
function setup ()
{
    // save the original image dimensions in global variables
    var image = document.getElementById("image");
    if (image.naturalWidth)
        imageWidth = image.naturalWidth;
    else
        imageWidth = image.width;
    if (image.naturalHeight)
        imageHeight = image.naturalHeight;
    else
        imageHeight = image.height;

    resizeAction();

    // the W3C event registration model doesn't seem to be completely 
    // supported by Konqueror
    //window.addEventListener('onresize', resizeAction, false);
    window.onresize = resizeAction;
    //window.attachEvent('resize', resizeAction);
}

// initial (load-time) entry point
function start ()
{
    // sleep briefly.  If I don't do this, Konqueror doesn't get 
    // computed style values correctly.
    setTimeout("setup()", 1);
}
