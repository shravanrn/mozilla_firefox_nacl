/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

/*
 * Tests bug 564900: Add folder specifically for downloads to Library left pane.
 * https://bugzilla.mozilla.org/show_bug.cgi?id=564900
 * This test visits various pages then opens the Library and ensures
 * that both the Downloads folder shows up and that the correct visits
 * are shown in it.
 */

var now = Date.now();

function test() {
  waitForExplicitFinish();

  let onLibraryReady = function(win) {
    // Add visits to compare contents with.
    let places = [
      { uri: NetUtil.newURI("http://mozilla.com"),
        visits: [ new VisitInfo(PlacesUtils.history.TRANSITION_TYPED) ]
      },
      { uri: NetUtil.newURI("http://google.com"),
        visits: [ new VisitInfo(PlacesUtils.history.TRANSITION_DOWNLOAD) ]
      },
      { uri: NetUtil.newURI("http://en.wikipedia.org"),
        visits: [ new VisitInfo(PlacesUtils.history.TRANSITION_TYPED) ]
      },
      { uri: NetUtil.newURI("http://ubuntu.org"),
        visits: [ new VisitInfo(PlacesUtils.history.TRANSITION_DOWNLOAD) ]
      },
    ]
    PlacesUtils.asyncHistory.updatePlaces(places, {
      handleResult() {},
      handleError() {
        ok(false, "gHistory.updatePlaces() failed");
      },
      handleCompletion() {
        // Make sure Downloads is present.
        isnot(win.PlacesOrganizer._places.selectedNode, null,
              "Downloads is present and selected");


        // Check results.
        let testURIs = ["http://ubuntu.org/", "http://google.com/"];
        for (let element of win.ContentArea.currentView
                                           .associatedElement.children) {
          is(element._shell.download.source.url, testURIs.shift(),
             "URI matches");
        }

        win.close();
        PlacesTestUtils.clearHistory().then(finish);
      }
    })
  }

  openLibrary(onLibraryReady, "Downloads");
}

function VisitInfo(aTransitionType) {
  this.transitionType =
    aTransitionType === undefined ?
      PlacesUtils.history.TRANSITION_LINK : aTransitionType;
  this.visitDate = now++ * 1000;
}
VisitInfo.prototype = {}
