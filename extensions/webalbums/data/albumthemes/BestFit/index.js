/*************************************************
* index.js
* Copyright (C) 2005-2006 Free Software Foundation, Inc.
* $Id$
*
* Scripts for DHTML photo album template index page
* The script in this file runs every time the page is reloaded or resized, and
* does the following:
*  - aligns and centers thumbnail pictures
*************************************************/

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
        

function getMaxContentWidth ( obj )
{
    var children = getChildren(obj);
    var y = 0;
    var contentWidth = 0;
    var parentWidth = getObjWidth(obj); //.parentNode);
    
    if (children.length > 0)
    {
        var i=0;
        while (i < children.length && contentWidth + getTotalWidth(children[i]) <= parentWidth)
        {
            contentWidth += getTotalWidth(children[i]);
            i++;
        }
    }

    return contentWidth;
}

// BUG: doesn't work in Konqueror
//  - margin-left and margin-right don't have values
//  - width doesn't update when page is resized
function center ()
{
    // locate all DIVs of class "centerh"
    var objs = getElementsByClass("div", "centerh");
    var wid;
    for (var i=0; i<objs.length; i++)
    {
        wid = getMaxContentWidth(objs[i]);
        setProperty(objs[i], "width", (wid)+"px");
    }
}

function align ()
{
    var children = getChildren(document.getElementById("thumbnailPanel"));
    if (children.length > 0)
    {
        var rowTop;
        var maxHeight;
        var tmp;
        
        // iterate over all children
        for (var i=0; i<children.length; i++)
        {
            rowTop = children[i].offsetTop;

            // find the maximum height of all thumbnails in this row
            maxHeight = -1;
            for (var j=i; (j<children.length) && (children[j].offsetTop==rowTop); j++)
            {
                tmp = getObjHeight(children[j]);
                if (tmp > maxHeight)
                    maxHeight = tmp;
            }
            
            // set the height on all thumbnails in this row to maxHeight
            while ((i < children.length) && (children[i].offsetTop == rowTop))
            {
                setProperty(children[i], "height", (maxHeight)+"px");
                setProperty(children[i], "marginBotton", (maxHeight - getObjHeight(children[i]))+"px");
                i++;
            }
        }
    }
}
            

function resizeAction ()
{
    center();

    /* if I'm using the floated block hack for thumbnail divs, then align them
       into rows */
    if (getProperty(getFirstChild(document.getElementById("thumbnailPanel")), "display") == "block")
        align();
}

function setupIndex ()
{
    resizeAction();

    window.onresize = resizeAction;
}

function startIndex ()
{
    // sleep briefly.  If I don't do this, Konqueror doesn't get 
    // computed style values correctly.
    setTimeout("setupIndex()", 1);
}

function showTip ( objid )
{
    var container = document.getElementById('thumb-' + objid);
    var obj = document.getElementById('tip-' + objid);
    if ( container != null && obj != null )
    {
        setProperty(obj, "top", (getTopOffset(container)-60)+"px");
        setProperty(obj, "left", (getLeftOffset(container)+getObjWidth(container))+"px");
        setProperty(obj, "display", "block");
    }
}

function hideTip ( objid )
{
    var obj = document.getElementById('tip-' + objid);
    if (obj != null)
        setProperty(document.getElementById('tip-' + objid), "display", "none");
}
