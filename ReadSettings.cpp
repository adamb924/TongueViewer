#include "ReadSettings.h"
#include <QtDebug>

ReadSettings::ReadSettings()
{
    dScaling = "";
}

ReadSettings::~ReadSettings()
{
}

bool ReadSettings::startElement(const QString &namespaceURI,
                                const QString &localName,
                                const QString &qName,
                                const QXmlAttributes &attributes)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    Q_UNUSED(qName);
    Q_UNUSED(attributes);
    currentString.clear();
    return true;
}

bool ReadSettings::endElement(const QString &namespaceURI,
                              const QString &localName,
                              const QString &qName)
{
    Q_UNUSED(namespaceURI);
    Q_UNUSED(localName);
    if(qName=="control") {
        controls << currentString;
    } else if(qName=="parameter") {
        parameters << currentString;
    } else if(qName=="dNTriangles") {
        dNEl = currentString.toULong();
    } else if(qName=="dNPoints") {
        dNPoints = currentString.toULong();
    } else if(qName=="sNTriangles") {
        sNEl = currentString.toULong();
    } else if(qName=="sNPoints") {
        sNPoints = currentString.toULong();
    } else if(qName=="dNHull") {
        nHull = currentString.toULong();
    } else if(qName=="sConnectivity") {
        sConnectivity = currentString;
    } else if(qName=="dConnectivity") {
        dConnectivity = currentString;
    } else if(qName=="dEigen") {
        dEigen = currentString;
    } else if(qName=="dMeans") {
        dMeans = currentString;
    } else if(qName=="dRange") {
        dRange = currentString;
    } else if(qName=="dScaling") {
        dScaling = currentString;
    } else if(qName=="dInitial") {
        dInitial = currentString;
    } else if(qName=="dHull") {
        dHull = currentString;
    } else if(qName=="annControlsToParameters") {
        annControlsToParameters = currentString;
    } else if(qName=="annParametersToControls") {
        annParametersToControls = currentString;
    }
    return true;
}

bool ReadSettings::characters(const QString &str)
{
    currentString += str;
    return true;
}

bool ReadSettings::fatalError(const QXmlParseException &exception)
{
    Q_UNUSED(exception);
    return true;
}
