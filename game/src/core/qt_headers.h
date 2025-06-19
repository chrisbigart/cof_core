#pragma once


#ifdef BUILD_WITH_UNREAL

#ifdef WIN32
#include "Windows/AllowWindowsPlatformTypes.h"
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wshadow"
#endif

#pragma push_macro("verify")
#undef verify

#pragma push_macro("check")
#undef check

//#pragma warning( disable : 4800)
//#pragma warning( disable : 4668)

#endif

#include <QBitArray>
#include <QColor>
#include <QCoreApplication>
#include <QDataStream>
#include <QBuffer>
#include <QDateTime>
#include <QDirIterator>
#include <QFile>
#include <QFileSystemWatcher>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>
#include <QList>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QRegularExpression>
#include <QSettings>
#include <QString>
#include <QTextStream>
#include <QTimer>
#include <QTranslator>
#include <QUuid>

#ifdef QT_NETWORK_LIB
#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QNetworkInterface>
#include <QNetworkReply>
#include <QNetworkRequest>
#endif


#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef BUILD_WITH_UNREAL

#pragma pop_macro("check")
#pragma pop_macro("verify")

#endif
