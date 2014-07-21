#ifndef READSETTINGS_H
#define READSETTINGS_H

//#include <QWidget>
#include <QXmlDefaultHandler>
#include <QWidget>

// class QXmlDefaultHandler;
class QStringList;

class ReadSettings : public QXmlDefaultHandler
{
public:
	ReadSettings();
	~ReadSettings();

	bool startElement(const QString &namespaceURI,
						const QString &localName,
						const QString &qName,
						const QXmlAttributes &attributes);
	bool endElement(const QString &namespaceURI,
						const QString &localName,
						const QString &qName);
	bool characters(const QString &str);
	bool fatalError(const QXmlParseException &exception);

	QString staticEl;

	QStringList controls, parameters;
	quint32 sNPoints, sNEl, dNPoints, dNEl, nHull;
	QString sConnectivity, dConnectivity, dEigen, dMeans, dRange, dInitial, dScaling, dHull, annControlsToParameters, annParametersToControls;

private:
	QString currentString;
	FILE *fid;
};

#endif
