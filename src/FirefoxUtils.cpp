//
//  FirefoxUtils.cpp
//  TimeCamp
//
//  Created by Aleksey Dvoryanskiy on 20.06.16
//  Modified by Georgiy Jenkovszky on 05.11.17
//  Ported to Qt by Karol Olszacki on 06.02.18
//

#include "FirefoxUtils.h"

#include <sys/stat.h>

#include <vector>
#include <algorithm>

#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QStandardPaths>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <cstring>
#include <QtCore/QThread>

#include "third-party/mozilla_lz4/lz4.h"

const char mozlz4_magic[] = {109, 111, 122, 76, 122, 52, 48, 0};  /* "mozLz40\0" */
const int decomp_size = 4;  /* 4 bytes size come after the header */
const size_t magic_size = sizeof mozlz4_magic;

bool FirefoxUtils::comparatorGreater(const std::pair<QString, time_t> &left, const std::pair<QString, time_t> &right)
{
    return left.second > right.second;
}

void *FirefoxUtils::readFileToMemory(const char *filename, size_t *readSize)
{
    size_t filesize = 0;
    void *returnValue = nullptr;
//    qDebug() << "[FirefoxUtils-readFileToMemory] Reading file: " << filename;
    FILE *fileDescriptor = fopen(filename, "rb");

    if (!fileDescriptor) {
        qDebug("[FirefoxUtils-readFileToMemory] No file descriptor");
        goto cleanup;
    }

    if (fseek(fileDescriptor, 0, SEEK_END) < 0) {
        qDebug("[FirefoxUtils-readFileToMemory] Can't read anymore");
        goto cleanup;
    }

    filesize = static_cast<size_t>(ftell(fileDescriptor));
    fseek(fileDescriptor, 0, SEEK_SET);

    if (!(returnValue = malloc(filesize))) {
        qDebug("[FirefoxUtils-readFileToMemory] Can't alloc values");
        goto cleanup;
    }

    if (filesize != fread(returnValue, 1, filesize, fileDescriptor)) {
        free(returnValue);
        returnValue = nullptr;
    }

    cleanup:
    if (fileDescriptor) {
        fclose(fileDescriptor);
    }

    if (returnValue && readSize) {
        *readSize = filesize;
    }

    return returnValue;
}

QString FirefoxUtils::parseJsRecoveryFilePath(const QString &recoveryFilePath)
{
    QFile file(recoveryFilePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return "";
    }

    QTextStream in(&file);

    QString content;

    while (!in.atEnd()) {
        content += in.readLine();
    }

    return content;
}

QString FirefoxUtils::parseJsonlz4RecoveryFilePath(const QString &recoveryFilePath)
{
    char *encryptedData = nullptr;
    char *decryptedData = nullptr;

    size_t readSize = 0;
    size_t outputBufferSize = 0;

    std::string str_recoveryFilePath = recoveryFilePath.toStdString();
    char *cstr_recoveryFilePath = new char[str_recoveryFilePath.length() + 1];
    std::strcpy(cstr_recoveryFilePath, str_recoveryFilePath.c_str());

    // QString -> std::wstring -> wchar_t* -> cast to char* | because old function works(TM)

    if (!(encryptedData = (char *) readFileToMemory(cstr_recoveryFilePath, &readSize))) {
        qDebug() << "[FirefoxUtils::parseJsonlz4RecoveryFilePath] Can't read file: " + recoveryFilePath;
        delete[] cstr_recoveryFilePath;
        return "";
    }
    delete[] cstr_recoveryFilePath;

    if (readSize < magic_size + decomp_size || memcmp(mozlz4_magic, encryptedData, magic_size) != 0) {
        qDebug() << "[FirefoxUtils::parseJsonlz4RecoveryFilePath] Unsupported file format: " + recoveryFilePath;
        free(encryptedData);
        return "";
    }

    size_t i = 0;
    for (i = magic_size; i < magic_size + decomp_size; i++) {
        outputBufferSize += (unsigned char) encryptedData[i] << (8 * (i - magic_size));
    }

    if (!(decryptedData = (char *) malloc(outputBufferSize))) {
        qDebug() << "[FirefoxUtils::parseJsonlz4RecoveryFilePath] Failed to allocate a buffer for an output.";
        free(encryptedData);
        return "";
    }

    int decryptedDataSize = LZ4_decompress_safe(encryptedData + i, decryptedData, (int) (readSize - i), (int) outputBufferSize);
    if (decryptedDataSize < 0) {
        qDebug() << "[FirefoxUtils::parseJsonlz4RecoveryFilePath] Failed to decompress a file: " + recoveryFilePath;
        free(decryptedData);
        free(encryptedData);
        return "";
    }
    QString qDecryptedData = QString(decryptedData);
    free(decryptedData);
    free(encryptedData);

    int indexOfLastProperChar = qDecryptedData.lastIndexOf("}");
    QString cutData = qDecryptedData.left(indexOfLastProperChar + 1);

//    qDebug() << "[FirefoxUtils::parseJsonlz4RecoveryFilePath] Successfully decompressed a file: " + recoveryFilePath;
//    // Debug: write to a D:/fx.json file
//    QString filename = "D:/fx.json";
//    QFile file(filename);
//    if (file.open(QIODevice::ReadWrite)) {
//        QTextStream stream(&file);
//        stream << cutData;
//    }
    return cutData;
}

QString FirefoxUtils::getFirefoxConfigFilePath()
{
#ifdef Q_OS_LINUX
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    QString firefoxPath = homeDir + "/.mozilla/firefox";
#elif defined(Q_OS_WIN)
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::HomeLocation).first();
    QString firefoxPath = homeDir + "/AppData/Roaming/Mozilla/Firefox/Profiles";
#else
    QString homeDir = QStandardPaths::standardLocations(QStandardPaths::GenericDataLocation).first();
    QString firefoxPath = homeDir + "/Firefox/Profiles";
#endif

    QDir firefoxDir(firefoxPath);
    if (!firefoxDir.isReadable()) {
        qInfo() << "Error: " + firefoxPath + " not found";
        return "";
    }

    firefoxDir.setFilter(QDir::Dirs | QDir::Readable);


    QStringList dirsMatches = {"*.default", "*.default-*"};
    QStringList dirsList = firefoxDir.entryList(dirsMatches, QDir::Dirs | QDir::Readable);
    if (dirsList.empty()) {
        qInfo() << "Error: *.default directory not found in " + firefoxPath;
        return "";
    }

    QString filename = firefoxPath + "/" + dirsList.first();

    QString sessionStoreFile = filename + "/sessionstore.js";
    QString recoveryFile = filename + "/sessionstore-backups/recovery.js";
    QString lz4RecoveryFile = filename + "/sessionstore-backups/recovery.jsonlz4";

    /* Here I need to check both files js and jsonlz4 and detect which one was written last. */
    QString configFilePath;
    struct stat attributes{};
    std::vector<std::pair<QString, time_t> > sessionFilesVector;

    if (QFile::exists(lz4RecoveryFile)) {
        stat((char *) lz4RecoveryFile.toStdWString().c_str(), &attributes);
        sessionFilesVector.emplace_back(lz4RecoveryFile, attributes.st_mtime);
        configFilePath = lz4RecoveryFile;
    }

    if (QFile::exists(sessionStoreFile)) {
        stat((char *) sessionStoreFile.toStdWString().c_str(), &attributes);
        sessionFilesVector.emplace_back(sessionStoreFile, attributes.st_mtime);
        configFilePath = sessionStoreFile;
    }

    if (QFile::exists(recoveryFile)) {
        stat((char *) recoveryFile.toStdWString().c_str(), &attributes);
        sessionFilesVector.emplace_back(recoveryFile, attributes.st_mtime);
        configFilePath = recoveryFile;
    }

    /* If vector is empty - return empty string. */
    if (sessionFilesVector.empty()) {
        return "";
    }
        /*
            If there is only one element then we don't have
            multiple sessions/Firefox versions running.
            No need to sort here.
        */
    else if (sessionFilesVector.size() == 1) {
        return sessionFilesVector.front().first;
    }

    /* Here we sort std::vector to determine which file was written last. */
    std::sort(sessionFilesVector.begin(), sessionFilesVector.end(), this->comparatorGreater);

    return sessionFilesVector.front().first;
}

QString FirefoxUtils::getCurrentURLFromFirefoxConfig(QString &jsonConfig, QString windowName)
{

    QJsonParseError error{};
    auto json = QJsonDocument::fromJson(jsonConfig.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error: " << error.errorString();
        return "";
    }
    auto jsonObject = json.object();
    auto jsonArray = jsonObject.value("properties").toArray();

    auto selectedWindowJson = jsonObject.value("selectedWindow");
    if (selectedWindowJson.isNull() || selectedWindowJson.isUndefined()) {
        qDebug() << "Failed getting 'selectedWindow'";
        return "";
    }

    int selectedWindow = selectedWindowJson.toInt() - 1;

    auto windowsJsonArray = jsonObject.value("windows");
    if (windowsJsonArray.isNull() || windowsJsonArray.isUndefined()) {
        qDebug() << "Failed getting 'windows'";
        return "";
    }

    auto windowJson = windowsJsonArray.toArray()[selectedWindow];
    if (windowJson.isNull() || windowJson.isUndefined()) {
        qDebug() << "Failed getting selected window";
        return "";
    }

    auto tabsJson = windowJson.toObject().value("tabs");
    if (tabsJson.isNull() || tabsJson.isUndefined()) {
        qDebug() << "Failed getting 'tabs'";
        return "";
    }

    auto tabsJsonArray = tabsJson.toArray();
    QString finalUrl;

    for (auto checkedTab: tabsJsonArray) {

        if (checkedTab.isNull() || checkedTab.isUndefined()) {
            qDebug() << "Failed getting selected tab";
            continue;
        }

        auto entriesJson = checkedTab.toObject().value("entries");
        if (entriesJson.isNull() || entriesJson.isUndefined()) {
            qDebug() << "Failed getting 'entries'";
            continue;
        }

        auto entriesJsonArray = entriesJson.toArray();
        int lastEntryIndex = entriesJsonArray.size() - 1;

        auto lastEntryJson = entriesJsonArray[lastEntryIndex];
        if (lastEntryJson.isNull() || lastEntryJson.isUndefined()) {
            qDebug() << "Failed getting last entry";
            continue;
        }

        auto lastEntryJsonObject = lastEntryJson.toObject();
        auto titleJson = lastEntryJsonObject.value("title");
        QString titleJsonQstring = titleJson.toString();
        if (titleJsonQstring != windowName) { // check if title matches windowName
//            qDebug() << "Bad window name, got: " << titleJsonQstring << ", looking for: " << windowName;
            continue;
        }

        auto urlJson = lastEntryJsonObject.value("url");
        if (urlJson.isNull() || urlJson.isUndefined()) {
            qDebug() << "Failed getting 'url'";
            continue;
        }
        finalUrl = urlJson.toString();
        break; // found the right one, we can skip
    }

    return finalUrl;
}

QString FirefoxUtils::getCurrentURLFromFirefox(QString windowName)
{
    QString content;
    activeUrl.clear();
    retries = 0;

    while (activeUrl.isEmpty() && retries < MAX_RETRIES) {
        if (recoveryFileExtension == "js") {
//        qDebug("[UForegroundApp::getAdditionalInfo] Parsing JS file.");
            content = parseJsRecoveryFilePath(recoveryFilePath);
        } else if (recoveryFileExtension == "jsonlz4") {
//        qDebug("[UForegroundApp::getAdditionalInfo] Parsing json lz4 compressed file.");
            content = parseJsonlz4RecoveryFilePath(recoveryFilePath);
        } else {
            qDebug("[UForegroundApp::getAdditionalInfo] Unsupported Firefox recovery file extension.");
            return "";
        }

        activeUrl = getCurrentURLFromFirefoxConfig(content, windowName).trimmed();
//        qDebug() << "[UForegroundApp::getAdditionalInfo] Firefox active URL: " << activeUrl;
        retries++;
    }

    return activeUrl;
}
FirefoxUtils::FirefoxUtils()
{
    recoveryFilePath = getFirefoxConfigFilePath();
    recoveryFileExtension = QFileInfo(recoveryFilePath).completeSuffix();
}
