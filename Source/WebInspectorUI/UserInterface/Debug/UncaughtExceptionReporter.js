/*
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

(function() {

const windowEvents = ["beforecopy", "copy", "click", "dragover", "focus"];
const documentEvents = ["focus", "blur", "resize", "keydown", "keyup", "mousemove", "pagehide", "contextmenu"];

function stopEventPropagation(event) {
    if (event.target.classList && event.target.classList.contains("bypass-event-blocking"))
        return;

    event.stopPropagation();
}

function blockEventHandlers() {
    // FIXME (151959): text selection on the sheet doesn't work for some reason.
    for (let name of windowEvents)
        window.addEventListener(name, stopEventPropagation, true);
    for (let name of documentEvents)
        document.addEventListener(name, stopEventPropagation, true);
}

function unblockEventHandlers() {
    for (let name of windowEvents)
        window.removeEventListener(name, stopEventPropagation, true);
    for (let name of documentEvents)
        document.removeEventListener(name, stopEventPropagation, true);
}

function handleUncaughtException(event) {
    let exceptionRecord = {
        message: event.message,
        url: parseURL(event.filename).lastPathComponent,
        lineNumber: event.lineno,
        columnNumber: event.colno
    };

    if (!window.__uncaughtExceptions)
        window.__uncaughtExceptions = [];

    const loadCompleted = window.__frontendCompletedLoad;
    const isFirstException = !window.__uncaughtExceptions.length;

    // If an uncaught exception happens after loading is done, only show
    // the first such exception. Many others may follow if internal
    // state has been corrupted, but these are unhelpful to report.
    if (!loadCompleted || isFirstException)
        window.__uncaughtExceptions.push(exceptionRecord);

    // If WebInspector.contentLoaded throws an uncaught exception, then these
    // listeners will not work correctly because the UI is not fully loaded.
    // Prevent any event handlers from running in an inconsistent state.
    if (isFirstException)
        blockEventHandlers();

    if (isFirstException && !loadCompleted) {
        // Signal that loading is done even though we can't guarantee that
        // evaluating code on the inspector page will do anything useful.
        // Without this, the frontend host may never show the window.
        if (InspectorFrontendHost)
            InspectorFrontendHost.loaded();

        // Don't tell InspectorFrontendAPI that loading is done, since it can
        // clear some of the error boilerplate page by accident.
    }

    createErrorSheet();
}

function dismissErrorSheet() {
    unblockEventHandlers();

    window.__sheetElement.remove();
    window.__sheetElement = null;
    window.__uncaughtExceptions = [];

    // Do this last in case WebInspector's internal state is corrupted.
    WebInspector.updateWindowTitle();

    // FIXME (151959): tell the frontend host to hide a draggable title bar.
}

function createErrorSheet() {
    // Early errors like parse errors may happen in the <head>, so attach
    // a body if none exists yet. Code below expects document.body to exist.
    if (!document.body)
        document.write("<body></body></html>");

    // FIXME (151959): tell the frontend host to show a draggable title bar.
    if (InspectorFrontendHost)
        InspectorFrontendHost.inspectedURLChanged("Internal Error");

    // Only allow one sheet element at a time.
    if (window.__sheetElement) {
        window.__sheetElement.remove();
        window.__sheetElement = null;
    }

    const loadCompleted = window.__frontendCompletedLoad;
    let firstException = window.__uncaughtExceptions[0];

    // Inlined from Utilities.js, because that file may not have loaded.
    function insertWordBreakCharacters(text) {
        return text.replace(/([\/;:\)\]\}&?])/g, "$1\u200b");
    }

    // This trampoline is necessary since none of our functions will be
    // in scope of an href="javascript:"-style evaluation.
    function handleLinkClick(event) {
        if (event.target.tagName !== "A")
            return;
        if (event.target.id === "dismiss-error-sheet")
            dismissErrorSheet();
    }

    let inspectedPageURL = "(unknown)";
    try {
        inspectedPageURL = WebInspector.frameResourceManager.mainFrame.url;
    } catch (e) { }

    let formattedErrorDetails = window.__uncaughtExceptions.map((entry) => `${entry.message} (at ${entry.url}:${entry.lineNumber}:${entry.columnNumber})`);
    let detailsForBugReport = formattedErrorDetails.map((line) => ` - ${line}`).join("\n");
    let encodedBugDescription = encodeURIComponent(`-------
Auto-generated details:

Inspected URL:        ${inspectedPageURL}
Loading completed:    ${!!loadCompleted}
Frontend User Agent:  ${window.navigator.userAgent}
Uncaught exceptions:
${detailsForBugReport}
-------

* STEPS TO REPRODUCE
1. What were you doing? Include setup or other preparations to reproduce the exception.
2. Include explicit, accurate, and minimal steps taken. Do not include extraneous or irrelevant steps.

* NOTES
Document any additional information that might be useful in resolving the problem, such as screen shots or other included attachments.
`);
    let encodedBugTitle = encodeURIComponent(`Uncaught Exception: ${firstException.message}`);
    let prefilledBugReportLink = `https://bugs.webkit.org/enter_bug.cgi?alias=&assigned_to=webkit-unassigned%40lists.webkit.org&attach_text=&blocked=&bug_file_loc=http%3A%2F%2F&bug_severity=Normal&bug_status=NEW&comment=${encodedBugDescription}&component=Web%20Inspector&contenttypeentry=&contenttypemethod=autodetect&contenttypeselection=text%2Fplain&data=&dependson=&description=&flag_type-1=X&flag_type-3=X&form_name=enter_bug&keywords=&op_sys=All&priority=P2&product=WebKit&rep_platform=All&short_desc=${encodedBugTitle}&version=WebKit%20Nightly%20Build`;
    let detailsForHTML = formattedErrorDetails.map((line) => `<li>${insertWordBreakCharacters(line)}</li>`).join("\n");

    let dismissOptionHTML = !loadCompleted ? "" : `<dt>A frivolous exception will not stop me!</dt>
        <dd><a class="bypass-event-blocking" id="dismiss-error-sheet">Click to close this view</a> and return
        to the Web Inspector without reloading. However, some things might not work without reloading if the error corrupted the Inspector's internal state.</dd>`;

    let sheetElement = window.__sheetElement = document.createElement("div");
    sheetElement.classList.add("sheet-container");
    sheetElement.innerHTML = `<div class="uncaught-exception-sheet">
    <h1>
    <img src="Images/Errors.svg">
    Web Inspector encountered an internal error.
    </h1>
    <dl>
        <dd>Usually, this is caused by a syntax error while modifying the Web Inspector
        UI, or running an updated frontend with out-of-date WebKit build.</dt>
        <dt>I didn't do anything...?</dt>
        <dd>If you don't think you caused this error to happen,
        <a href="${prefilledBugReportLink}">click to file a pre-populated
        bug with this information</a>. It is possible that someone else broke it by accident.</dd>
        <dt>Oops, can I try again?</dt>
        <dd><a href="javascript:window.location.reload()">Click to reload the Inspector</a>
        again after making local changes.</dd>
        ${dismissOptionHTML}
    </dl>
    <h2>
    <img src="Images/Console.svg">
    These uncaught exceptions caused the problem:
    </h2>
    <p><ul>${detailsForHTML}</ul></p>
    </div>`;

    sheetElement.addEventListener("click", handleLinkClick, true);
    document.body.appendChild(sheetElement);
}

window.addEventListener("error", handleUncaughtException);

})();
