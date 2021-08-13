// Installer script for qt.
// Based on https://github.com/rabits/dockerfiles/blob/master/5.13-desktop/extract-qt-installer.sh
// See https://doc.qt.io/qtinstallerframework/noninteractive.html

function Controller() {
    //installer.autoRejectMessageBoxes();
    // installer.autoRejectMessageBoxes;
    installer.setMessageBoxAutomaticAnswer("OverwriteTargetDirectory", QMessageBox.Yes);
    installer.setMessageBoxAutomaticAnswer("stopProcessesForUpdates", QMessageBox.Ignore);
    installer.setMessageBoxAutomaticAnswer("TargetDirectoryInUse", QMessageBox.OK);
    installer.setMessageBoxAutomaticAnswer("cancelInstallation", QMessageBox.Yes);

    installer.installationFinished.connect(function() {
        gui.clickButton(buttons.NextButton);
    });
}

Controller.prototype.WelcomePageCallback = function() {
    console.log("Welcome Page");
    gui.clickButton(buttons.NextButton, 1500);
}

Controller.prototype.CredentialsPageCallback = function() {
    console.log("Credentials Page");
    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.IntroductionPageCallback = function() {
    console.log("Introduction Page");
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.TargetDirectoryPageCallback = function()
{
    console.log("Target Directory Page");
    // gui.currentPageWidget().TargetDirectoryLineEdit.setText("/opt/Qt");
    gui.clickButton(buttons.NextButton);
}

/// Question for tracking usage data, refuse it
Controller.prototype.DynamicTelemetryPluginFormCallback = function() {
    console.log("Tracking usage data");
    var widget = gui.currentPageWidget();
    var radioButtons = widget.TelemetryPluginForm.statisticGroupBox;
    radioButtons.disableStatisticRadioButton.checked = true;
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.ComponentSelectionPageCallback = function() {
    var components = [
      "qt.qt5.5142.gcc_64"
    ]
    console.log("Select components");
    
    var page = gui.pageWidgetByObjectName("ComponentSelectionPage");
    // if CategoryGroupBox is visible, check one of the checkboxes
    // and click fetch button before selecting any components
    var groupBox = gui.findChild(page, "CategoryGroupBox");
    if (groupBox) {
        console.log("groupBox found");
        // findChild second argument is the display name of the checkbox
        var checkBox = gui.findChild(page, "Archive");
        if (checkBox) {
            console.log("checkBox found");
            console.log("checkBox name: " + checkBox.text);
            if (checkBox.checked == false) {
                checkBox.click();
                var fetchButton = gui.findChild(page, "FetchCategoryButton");
                if (fetchButton) {
                    console.log("fetchButton found");
                    fetchButton.click();
                } else {
                    console.log("fetchButton NOT found");
                }
            }
        } else {
            console.log("checkBox NOT found");
        }
    } else {
        console.log("groupBox NOT found");
    }

    var widget = gui.currentPageWidget();
//    console.log(widget.keys);
    widget.deselectAll();
    for (var i=0; i < components.length; i++){
        widget.selectComponent(components[i]);
        console.log("selected: " + components[i])
    }
    gui.clickButton(buttons.NextButton);
}

Controller.prototype.LicenseAgreementPageCallback = function() {
    console.log("Accept license agreement");
    var widget = gui.currentPageWidget();
    if (widget != null) {
        widget.AcceptLicenseRadioButton.setChecked(true);
    }
    gui.clickButton(buttons.NextButton);
    //gui.clickButton(buttons.CancelButton);
}

Controller.prototype.ReadyForInstallationPageCallback = function() {
    console.log("Ready to install");
    gui.clickButton(buttons.CommitButton);
}

Controller.prototype.FinishedPageCallback = function() {
    console.log("Done.");
    var widget = gui.currentPageWidget();
    if (widget.LaunchQtCreatorCheckBoxForm) {
        widget.LaunchQtCreatorCheckBoxForm.launchQtCreatorCheckBox.setChecked(false);
    }
    gui.clickButton(buttons.FinishButton);
}
