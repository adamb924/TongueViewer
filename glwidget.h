#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QGLWidget>

#define M_DYNAMIC 0
#define M_STATIC 1
#define M_PSCORE 2
#define M_CSCORE 3

#define EIGEN(i,j) (*(eigen+(j*ndim)+i))
#define DATA(i,j) (*(data+(j*nreteigen)+i))
#define NEWDATA(i,j) (*(newdata+(j*nreteigen)+i))

#define CENTER(i,j) (*(hull+((i)*nreteigen)+(j)))
#define NORMAL_REF(i,j) ((hull+nhull*nreteigen+((i)*nreteigen)+(j)))

// address of layer i, row j
#define LW_ROW(i,j) *(w+(i)) + (j)* *(n+(i)-1)
// bias of layer i, neuron j
#define BIAS(i,j) *( *(b+(i)) + (j))
// address of layer i output
#define OUTPUT_ADDR(i) (*(output+(i)) )
// layer i, output j
#define OUTPUT(i,j) *( *(output+(i)) + (j) )
// input minimum of dimension i
#define IN_MIN(i) *(inmin+(i))
// input maximum of dimension i
#define IN_MAX(i) *(inmax+(i))
// output minimum of dimension i
#define OUT_MIN(i) *(outmin+(i))
// output maximum of dimension i
#define OUT_MAX(i) *(outmax+(i))

class GLWidget : public QGLWidget
{
	Q_OBJECT

public:
	GLWidget(QWidget *parent = 0);
	~GLWidget();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;
	void SetVariables(quint32 nel, quint16 *el, float *no);
	void saveView(QString filename);


public slots:
	void setXRotation(int angle);
	void setYRotation(int angle);
	void setZRotation(int angle);
	void TakePictures();
	void normalizeAngle(int *angle);

signals:
	void xRotationChanged(int angle);
	void yRotationChanged(int angle);
	void zRotationChanged(int angle);

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int width, int height);
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);

	void Log(QString err);

private:

	QColor bgColor;

	// model parameters
	unsigned char mode;
	float *nodes;
	quint16 *elements;
	quint32 nelements;

	GLuint object;
	int xRot;
	int yRot;
	int zRot;
	QPoint lastPos;
	QColor trolltechGreen;
	QColor trolltechPurple;
};

#endif
