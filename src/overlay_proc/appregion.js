if (window.jQuery) {
    window.overlayAppRegionSelected = new Set();
    window.overlayAppRegionDrag = false;
    function regionAddNode(e) {
        try {
            window.overlayAppRegionSelected.add(event.target);
            var appRegion = false;
            for (let item of overlayAppRegionSelected)
            {
                $(item).parents().each(function (i, v) {
                    try {
                        if ($(v).css('-webkit-app-region') == 'drag') {
                            appRegion = true;
                        }
                    }
                    catch (e) {/*console.log(e);*/ }
                });
            }
            window.overlayAppRegionDrag = appRegion;
        }
        catch (e) {/*console.log(e);*/ }
    };
    function regionDeleteNode(e) {
        try {
            window.overlayAppRegionSelected.delete(event.target);
            window.overlayAppRegionDrag = false;
        }
        catch (e) {/*console.log(e);*/ }
    };
    try {
        $('div').mouseenter(regionAddNode);
        $('div').mouseout(regionDeleteNode);
        $('td').mouseenter(regionAddNode);
        $('td').mouseout(regionDeleteNode);
        $('a').mouseenter(regionAddNode);
        $('a').mouseout(regionDeleteNode);
        $('span').mouseenter(regionAddNode);
        $('span').mouseout(regionDeleteNode);
    }
    catch (e) {/*console.log(e);*/ }
}
