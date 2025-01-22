function Component()
{
}

Component.prototype.createOperations = function()
{
    component.createOperations();

    if (systemInfo.productType === "windows") {
        component.addOperation("CreateShortcut", 
            "@TargetDir@/proxyserver.exe",
            "@StartMenuDir@/Censortracker Proxy Server.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@/proxyserver.exe",
            "description=Censortracker Proxy Server");
            
        component.addOperation("CreateShortcut",
            "@TargetDir@/proxyserver.exe",
            "@DesktopDir@/Censortracker Proxy Server.lnk",
            "workingDirectory=@TargetDir@",
            "iconPath=@TargetDir@/proxyserver.exe",
            "description=Censortracker Proxy Server");
    }
} 