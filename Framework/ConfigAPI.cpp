// For conditions of distribution and use, see copyright notice in license.txt

#include "StableHeaders.h"
#include "Framework.h"
#include "Platform.h"
#include "ConfigAPI.h"

#include "LoggingFunctions.h"

#include <QSettings>
#include <QString>
#include <QDir>
#include <QDebug>

ConfigAPI::ConfigAPI(Foundation::Framework *framework) :
    QObject(framework),
    framework_(framework),
    configFolder_("")
{
    framework_->RegisterDynamicObject("config", this);
}

void ConfigAPI::SetApplication(const QString &applicationOrganization, const QString &applicationName, const QString &applicationVersion)
{
    applicationOrganization_ = applicationOrganization;
    applicationName_ = applicationName;
    applicationVersion_ = applicationVersion;
}

void ConfigAPI::PrepareDataFolder(const QString &configFolderName)
{
    if (!framework_->GetPlatform())
    {
        LogFatal("ConfigAPI::PrepareDataFolder: Framworks Platform objects has not been initialized yet, aborting!");
        return;
    }

    /// \todo Should get the unicode GetApplicationDataDirectoryW() QString::fromStdWString seems to give a weird linker error, types wont match?
    QString applicationDataDir = QString::fromStdString(framework_->GetPlatform()->GetApplicationDataDirectory());
    
    // Prepare application data dir path
    applicationDataDir.replace("\\", "/");
    if (!applicationDataDir.endsWith("/"))
        applicationDataDir.append("/");

    // Create directory if does not exist
    QDir configDataDir(applicationDataDir);
    if (!configDataDir.exists(configFolderName))
        configDataDir.mkdir(configFolderName);
    configDataDir.cd(configFolderName);

    configFolder_ = configDataDir.absolutePath();

    // Be sure the path ends with a forward slash
    if (!configFolder_.endsWith("/"))
        configFolder_.append("/");
}

QString ConfigAPI::GetFilePath(const QString &file)
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::GetFilePath: Config folder has not been prepared, returning empty string.");
        return "";
    }

    QString filePath = configFolder_ + file;
    if (!filePath.endsWith(".ini"))
        filePath.append(".ini");
    return filePath;
}

void ConfigAPI::PrepareString(QString &str)
{
    if (!str.isEmpty())
    {
        str = str.trimmed().toLower();  // Remove spaces from start/end, force to lower case
        str = str.replace(" ", "_");    // Replace ' ' with '_', so we don't get %20 in the config as spaces
        str = str.replace("=", "_");    // Replace '=' with '_', as = has special meaning in .ini files
        str = str.replace("/", "_");    // Replace '/' with '_', as / has a special meaning in .ini file keys/sections. Also file name cannot have a forward slash.
    }
}

bool ConfigAPI::HasValue(QString file, QString section, QString key)
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::Get: Config folder has not been prepared, returning empty string.");
        return "";
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    QSettings config(GetFilePath(file), QSettings::IniFormat);
    if (!section.isEmpty())
        key = section + "/" + key;
    return config.allKeys().contains(key);
}

QVariant ConfigAPI::Get(QString file, QString section, QString key, const QVariant &defaultValue)
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::Get: Config folder has not been prepared, returning empty string.");
        return "";
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    QSettings config(GetFilePath(file), QSettings::IniFormat);
    if (section.isEmpty())
        return config.value(key, defaultValue);
    else
        return config.value(section + "/" + key, defaultValue);
}

void ConfigAPI::Set(QString file, QString section, QString key, const QVariant &value)
{
    if (configFolder_.isEmpty())
    {
        LogError("ConfigAPI::Set: Config folder has not been prepared, not storing value to config empty string.");
        return;
    }

    PrepareString(file);
    PrepareString(section);
    PrepareString(key);

    QSettings config(GetFilePath(file), QSettings::IniFormat);
    if (!config.isWritable())
        return;
    if (section.isEmpty())
        config.setValue(key, value);
    else
        config.setValue(section + "/" + key, value);
    config.sync();
}
