#include <QtGui>
#include <QtOpenGL>

#include <math.h>

#include "glwidget.h"

void cross(float a1,float a2, float a3, float b1, float b2, float b3,float *i, float *j, float *k)
{
    *i = a2*b3 - a3*b2;
    *j = a3*b1 - a1*b3;
    *k = a1*b2 - a2*b1;
}


GLWidget::GLWidget(QWidget *parent)
    : QGLWidget(parent)
{
    object = 0;
    xRot = 0;
    yRot = 0;
    zRot = 0;

    bgColor = QColor(0, 0, 80, 255);

    nelements=0;

    nodes=NULL;
    elements=NULL;

    trolltechGreen = QColor::fromCmykF(0.40, 0.0, 1.0, 0.0);
    trolltechPurple = QColor::fromCmykF(0.39, 0.39, 0.0, 0.0);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    glDeleteLists(object, 1);
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(50, 50);
}

QSize GLWidget::sizeHint() const
{
    return QSize(400, 400);
}

void GLWidget::setXRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != xRot) {
        xRot = angle;
        emit xRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setYRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != yRot) {
        yRot = angle;
        emit yRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::setZRotation(int angle)
{
    normalizeAngle(&angle);
    if (angle != zRot) {
        zRot = angle;
        emit zRotationChanged(angle);
        updateGL();
    }
}

void GLWidget::initializeGL()
{
    qglClearColor(bgColor);

    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);

    glDisable(GL_CULL_FACE);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glTranslated(0.0, 0.0, -10.0);
    glRotated(xRot / 16.0, 1.0, 0.0, 0.0);
    glRotated(yRot / 16.0, 0.0, 1.0, 0.0);
    glRotated(zRot / 16.0, 0.0, 0.0, 1.0);
    glCallList(object);


    quint32 i;

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_AUTO_NORMAL);

    // Create light components
    GLfloat ambientLight[] = { 0.2f, 0.2f, 0.2f, 0.0f };
    GLfloat diffuseLight[] = { 0.5f, 0.5f, 0.5, 0.0f };
    GLfloat specularLight[] = { 0.5f, 0.5f, 0.5f, 0.0f };
    GLfloat position[] = {0.0, 0.0, 75.0, 1.0};

    // Assign created components to GL_LIGHT0
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specularLight);
    glLightfv(GL_LIGHT0, GL_POSITION, position);


    glEnable(GL_LIGHT1);
    GLfloat position1[] = {0.0, 0.0, -50.0, 1.0};
    GLfloat diffuseLight1[] = { 0.4f, 0.4f, 0.4, 0.0f };
    glLightfv(GL_LIGHT1, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, diffuseLight1);
    glLightfv(GL_LIGHT1, GL_POSITION, position1);

    glEnable(GL_LIGHT2);
    GLfloat position2[] = {-50.0, 0.0, 0.0, 1.0};
    GLfloat diffuseLight2[] = { 0.4f, 0.4f, 0.4, 0.0f };
    glLightfv(GL_LIGHT2, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, diffuseLight2);
    glLightfv(GL_LIGHT2, GL_POSITION, position2);

    if(nelements==0 || elements==NULL || nodes==NULL)
        return;

    float x1, x2, x3;
    float y1, y2, y3;
    float z1, z2, z3;
    float nx, ny, nz;

    glDisable(GL_CULL_FACE);

    // draw the triangles
    glPushMatrix();
    glTranslatef(-56.9614f,-47.0304f,-36.4743f);

    //	glBegin(GL_POINTS);
    glBegin(GL_TRIANGLES);
    for(i=0; i<nelements; i++)
    {
        x1 = *(nodes + 3*(*(elements+(3*i)+0)) + 0);
        y1 = *(nodes + 3*(*(elements+(3*i)+0)) + 1);
        z1 = *(nodes + 3*(*(elements+(3*i)+0)) + 2);

        x2 = *(nodes + 3*(*(elements+(3*i)+1)) + 0);
        y2 = *(nodes + 3*(*(elements+(3*i)+1)) + 1);
        z2 = *(nodes + 3*(*(elements+(3*i)+1)) + 2);

        x3 = *(nodes + 3*(*(elements+(3*i)+2)) + 0);
        y3 = *(nodes + 3*(*(elements+(3*i)+2)) + 1);
        z3 = *(nodes + 3*(*(elements+(3*i)+2)) + 2);

        cross(x3-x1,y3-y1,z3-z1,x2-x1,y2-y1,z2-z1,&nx,&ny,&nz);

        glColor3f(0.4f,0.4f,0.4f);
        glNormal3f(nx,ny,nz);
        glVertex3f(x1,y1,z1);
        glVertex3f(x2,y2,z2);
        glVertex3f(x3,y3,z3);

    }
    glEnd();

    glPopMatrix();
}

void GLWidget::resizeGL(int width, int height)
{
    int dimen = 75;
    glViewport(0, 0, width, height);
    float aspect = (float)width/(float)height;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-1*dimen*aspect, dimen*aspect, dimen, -1*dimen, -100.0, 100.0);
    glMatrixMode(GL_MODELVIEW);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if (event->buttons() & Qt::LeftButton) {
        //		setXRotation(xRot + dy);
        //		setYRotation(yRot + dx);
        setXRotation(xRot + 8 * dy);
        setYRotation(yRot + 8 * dx);
    } else if (event->buttons() & Qt::RightButton) {
        //		setXRotation(xRot + dy);
        //		setZRotation(zRot + dx);
        setXRotation(xRot + 8 * dy);
        setZRotation(zRot + 8 * dx);
    }
    lastPos = event->pos();
}

void GLWidget::normalizeAngle(int *angle)
{
    while (*angle < 0)
        *angle += 360 * 16;
    while (*angle > 360 * 16)
        *angle -= 360 * 16;
}

void GLWidget::SetVariables(quint32 nel, quint16 *el, float *no)
{
    nelements = nel;
    elements = el;
    nodes = no;
}

void GLWidget::Log(QString err)
{
    FILE *fid;
    QByteArray ba;

    fid = fopen("error.log","a+");
    ba = err.toLatin1();
    fprintf(fid,ba.data());
    fprintf(fid,"\n");
    fclose(fid);
}

void GLWidget::TakePictures()
{
    quint16 margin=20;

    bool ok;
    QString fn = QInputDialog::getText(this, tr("Enter a filename"),
                                       tr("Filename (.png assumed):"), QLineEdit::Normal,
                                       tr(""), &ok);
    if (!ok || fn.isEmpty())
    {
        return;
    }

    int *vp = (int*)malloc(sizeof(int)*4);
    int w, h, i, j;
    int w1, w2, w3;
    int nw, nh;
    int mn, mx;
    int mn1, mn2, mn3;
    int mx1, mx2, mx3;
    quint32 *buf, *buf1, *buf2, *buf3;

    glGetIntegerv(GL_VIEWPORT,vp);
    w = *(vp+2);
    h = *(vp+3);

    buf1 = (quint32*)malloc(sizeof(quint32)*w*h);
    buf2 = (quint32*)malloc(sizeof(quint32)*w*h);
    buf3 = (quint32*)malloc(sizeof(quint32)*w*h);


    xRot = 270*16;
    emit xRotationChanged(xRot);
    yRot = 0;
    emit yRotationChanged(yRot);
    zRot = 270*16;
    emit zRotationChanged(zRot);
    updateGL();
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,buf1);

    xRot = 90*16;
    emit xRotationChanged(xRot);
    yRot = 180*16;
    emit yRotationChanged(yRot);
    zRot = 0;
    emit zRotationChanged(zRot);
    updateGL();
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,buf2);

    xRot = 0*16;
    emit xRotationChanged(xRot);
    yRot = 180*16;
    emit yRotationChanged(yRot);
    zRot = 0;
    emit zRotationChanged(zRot);
    updateGL();
    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,buf3);

    /*
    FILE *dmp;
    dmp = fopen("dmp1.bin","wb");
    fwrite(buf1,sizeof(char),w*h*4,dmp);
    fclose(dmp);
    dmp = fopen("dmp2.bin","wb");
    fwrite(buf2,sizeof(char),w*h*4,dmp);
    fclose(dmp);
    dmp = fopen("dmp3.bin","wb");
    fwrite(buf3,sizeof(char),w*h*4,dmp);
    fclose(dmp);
*/

    mn=0;
    mx=0;
    mn1=999999;
    mx1=0;
    mn2=999999;
    mx2=0;
    mn3=999999;
    mx3=0;

    for(i=0;i<h;i++)
    {
        for(j=0; j<w; j++)
        {
            if( *(buf1+(w*i)+j+0)==0xFF500000 )
            {
                *(buf1+(w*i)+j)= 0;
            }
            else
            {
                if(mn==0)
                {
                    mn=i;
                }
                if(mx<i)
                {
                    mx=i;
                }
                if(mn1>j)
                {
                    mn1=j;
                }
                if(mx1 < j)
                {
                    mx1 = j;
                }
            }
        }

        for(j=0; j<w; j++)
        {
            if( *(buf2+(w*i)+j+0)==0xFF500000 )
            {
                *(buf2+(w*i)+j)= 0;
            }
            else
            {
                if(mn==0)
                {
                    mn=i;
                }
                if(mx<i)
                {
                    mx=i;
                }
                if(mn2>j)
                {
                    mn2=j;
                }
                if(mx2 < j)
                {
                    mx2 = j;
                }
            }
        }

        for(j=0; j<w; j++)
        {
            if( *(buf3+(w*i)+j+0)==0xFF500000 )
            {
                *(buf3+(w*i)+j)= 0;
            }
            else
            {
                if(mn==0)
                {
                    mn=i;
                }
                if(mx<i)
                {
                    mx=i;
                }
                if(mn3>j)
                {
                    mn3=j;
                }
                if(mx3 < j)
                {
                    mx3 = j;
                }
            }
        }
    }

    w1 = mx1-mn1;
    w2 = mx2-mn2;
    w3 = mx3-mn3;

    nh = mx-mn+1;
    nw = w1 + margin + w2 + margin + w3;
    buf = (quint32*)malloc(sizeof(quint32)*margin);

    for(i=0; i<margin; i++) { *(buf+i) = 0; }

    FILE *fid;
    fid = fopen("buf.bin","wb");

    for(i=mn; i<mx; i++)
    {
        fwrite(buf1+w*i+mn1,4,w1,fid);
        fwrite(buf,4,margin,fid);
        fwrite(buf2+w*i+mn2,4,w2,fid);
        fwrite(buf,4,margin,fid);
        fwrite(buf3+w*i+mn3,4,w3,fid);
    }
    fclose(fid);
    free(buf);

    buf = (quint32*)malloc(sizeof(quint32)*nw*nh);
    QFile file("buf.bin");
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening buf.bin"; return; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for(i=0; i<nw*nh; i++)
    {
        in >> *(buf+i);
    }
    file.close();


    QImage *img = new QImage((unsigned char*)buf,nw,nh,QImage::Format_ARGB32);
    QImage img2 = img->rgbSwapped();
    img2 = img2.mirrored(false,true);
    img2.save(fn + ".png");

    free(vp);
    free(buf);
    free(buf1);
    free(buf2);
    free(buf3);
}

void GLWidget::saveView(QString filename)
{
    int *vp = (int*)malloc(sizeof(int)*4);
    int w, h;
    quint32 *buf;
    glGetIntegerv(GL_VIEWPORT,vp);
    w = *(vp+2);
    h = *(vp+3);

    buf = (quint32*)malloc(sizeof(quint32)*w*h);

    glReadPixels(0,0,w,h,GL_RGBA,GL_UNSIGNED_BYTE,buf);

    for(int i=0; i<w*h; i++)
        if( *(buf+i) == 0xFF500000 )
            *(buf+i) = 0xFFFFFFFF;

    QImage img((unsigned char*)buf,w,h,QImage::Format_ARGB32);
    img.mirrored(false,true).save(filename);

    free(buf);
    free(vp);
}
