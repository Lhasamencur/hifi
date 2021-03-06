//
//  HTTPManager.cpp
//  libraries/embedded-webserver/src
//
//  Created by Stephen Birarda on 1/16/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMimeDatabase>
#include <QtNetwork/QTcpSocket>

#include "HTTPConnection.h"
#include "HTTPManager.h"

bool HTTPManager::handleHTTPRequest(HTTPConnection* connection, const QUrl& url) {
    if (_requestHandler && _requestHandler->handleHTTPRequest(connection, url)) {
        // this request was handled by our _requestHandler object
        // so we don't need to attempt to do so in the document root
        return true;
    }
    
    // check to see if there is a file to serve from the document root for this path
    QString subPath = url.path();
    
    // remove any slash at the beginning of the path
    if (subPath.startsWith('/')) {
        subPath.remove(0, 1);
    }
    
    QString filePath;
    
    if (QFileInfo(_documentRoot + subPath).isFile()) {
        filePath = _documentRoot + subPath;
    } else if (subPath.size() > 0 && !subPath.endsWith('/')) {
        // this could be a directory with a trailing slash
        // send a redirect to the path with a slash so we can
        QString redirectLocation = '/' + subPath + '/';

        if (!url.query().isEmpty()) {
            redirectLocation += "?" + url.query();
        }
        
        QHash<QByteArray, QByteArray> redirectHeader;
        redirectHeader.insert(QByteArray("Location"), redirectLocation.toUtf8());
        
        connection->respond(HTTPConnection::StatusCode301, "", HTTPConnection::DefaultContentType, redirectHeader);
    }
    
    // if the last thing is a trailing slash then we want to look for index file
    if (subPath.endsWith('/') || subPath.size() == 0) {
        QStringList possibleIndexFiles = QStringList() << "index.html" << "index.shtml";
        
        foreach (const QString& possibleIndexFilename, possibleIndexFiles) {
            if (QFileInfo(_documentRoot + subPath + possibleIndexFilename).exists()) {
                filePath = _documentRoot + subPath + possibleIndexFilename;
                break;
            }
        }
    }
    
    if (!filePath.isEmpty()) {
        // file exists, serve it
        static QMimeDatabase mimeDatabase;
        
        QFile localFile(filePath);
        localFile.open(QIODevice::ReadOnly);
        QByteArray localFileData = localFile.readAll();
        
        QFileInfo localFileInfo(filePath);
        
        if (localFileInfo.completeSuffix() == "shtml") {
            // this is a file that may have some SSI statements
            // the only thing we support is the include directive, but check the contents for that
            
            // setup our static QRegExp that will catch <!--#include virtual ... --> and <!--#include file .. --> directives
            const QString includeRegExpString = "<!--\\s*#include\\s+(virtual|file)\\s?=\\s?\"(\\S+)\"\\s*-->";
            QRegExp includeRegExp(includeRegExpString);
            
            int matchPosition = 0;
            
            QString localFileString(localFileData);
            
            while ((matchPosition = includeRegExp.indexIn(localFileString, matchPosition)) != -1) {
                // check if this is a file or vitual include
                bool isFileInclude = includeRegExp.cap(1) == "file";
                
                // setup the correct file path for the included file
                QString includeFilePath = isFileInclude
                    ? localFileInfo.canonicalPath() + "/" + includeRegExp.cap(2)
                    : _documentRoot + includeRegExp.cap(2);
                
                QString replacementString;
                
                if (QFileInfo(includeFilePath).isFile()) {
                    
                    QFile includedFile(includeFilePath);
                    includedFile.open(QIODevice::ReadOnly);
                    
                    replacementString = QString(includedFile.readAll());
                } else {
                    qDebug() << "SSI include directive referenced a missing file:" << includeFilePath;
                }
                
                // replace the match with the contents of the file, or an empty string if the file was not found
                localFileString.replace(matchPosition, includeRegExp.matchedLength(), replacementString);
                
                // push the match position forward so we can check the next match
                matchPosition += includeRegExp.matchedLength();
            }
            
            localFileData = localFileString.toLocal8Bit();
        }
        
        connection->respond(HTTPConnection::StatusCode200, localFileData,
                            qPrintable(mimeDatabase.mimeTypeForFile(filePath).name()));
    } else {
        
        // respond with a 404
        connection->respond(HTTPConnection::StatusCode404, "Resource not found.");
    }
    
    return true;
}

HTTPManager::HTTPManager(quint16 port, const QString& documentRoot, HTTPRequestHandler* requestHandler, QObject* parent) :
    QTcpServer(parent),
    _documentRoot(documentRoot),
    _requestHandler(requestHandler)
{
    // start listening on the passed port
    if (!listen(QHostAddress("0.0.0.0"), port)) {
        qDebug() << "Failed to open HTTP server socket:" << errorString();
        return;
    }
    
    // connect the connection signal
    connect(this, SIGNAL(newConnection()), SLOT(acceptConnections()));
}

void HTTPManager::acceptConnections() {
    QTcpSocket* socket;
    while ((socket = nextPendingConnection()) != 0) {
        new HTTPConnection(socket, this);
    }
}
