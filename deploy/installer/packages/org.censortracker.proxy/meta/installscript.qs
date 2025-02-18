function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", 
            "@TargetDir@/CensorTrackerProxy.exe",
            "@StartMenuDir@/Censor Tracker Proxy.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@/CensorTrackerProxy.exe",
            "description=Censor Tracker Proxy");
            
        component.addOperation("CreateShortcut",
            "@TargetDir@/CensorTrackerProxy.exe",
            "@DesktopDir@/Censor Tracker Proxy.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@/CensorTrackerProxy.exe",
            "description=Censor Tracker Proxy");
    }
} 