#include <QtWidgets>
#include <QErrorMessage>
#include <QXmlInputSource>
#include <QtDebug>
#include <QProcess>

#include "glwidget.h"
#include "window.h"
#include "ReadSettings.h"

#define min(X, Y)  ((X) < (Y) ? (X) : (Y))
#define max(X, Y)  ((X) > (Y) ? (X) : (Y))

// address of layer i, row j
#define LW_ROW_PC(i,j) *(wPC+(i)) + (j)* *(nPC+(i)-1)
// bias of layer i, neuron j
#define BIAS_PC(i,j) *( *(bPC+(i)) + (j))
// address of layer i output
#define OUTPUT_ADDR_PC(i) (*(outputPC+(i)) )
// layer i, output j
#define OUTPUT_PC(i,j) *( *(outputPC+(i)) + (j) )
// input minimum of dimension i
#define IN_MIN_PC(i) *(inminPC+(i))
// input maximum of dimension i
#define IN_MAX_PC(i) *(inmaxPC+(i))
// output minimum of dimension i
#define OUT_MIN_PC(i) *(outminPC+(i))
// output maximum of dimension i
#define OUT_MAX_PC(i) *(outmaxPC+(i))

// address of layer i, row j
#define LW_ROW_CP(i,j) *(wCP+(i)) + (j)* *(nCP+(i)-1)
// bias of layer i, neuron j
#define BIAS_CP(i,j) *( *(bCP+(i)) + (j))
// address of layer i output
#define OUTPUT_ADDR_CP(i) (*(outputCP+(i)) )
// layer i, output j
#define OUTPUT_CP(i,j) *( *(outputCP+(i)) + (j) )
// input minimum of dimension i
#define IN_MIN_CP(i) *(inminCP+(i))
// input maximum of dimension i
#define IN_MAX_CP(i) *(inmaxCP+(i))
// output minimum of dimension i
#define OUT_MIN_CP(i) *(outminCP+(i))
// output maximum of dimension i
#define OUT_MAX_CP(i) *(outmaxCP+(i))

#define TIME_P(i) ( *(instP + (1+nParameters)*(i) ))
#define LEVEL_P(i,c) ( *(instP + (1+nParameters)*(i) +(c) + 1))

#define TIME_C(i) ( *(instC + (1+nControls)*(i) ))
#define LEVEL_C(i,c) ( *(instC + (1+nControls)*(i) +(c) + 1))

#define VIEW_TIME(i) ( *(instView + 4*(i) + 0) )
#define ROT_X(i) ( *(instView + 4*(i) + 1) )
#define ROT_Y(i) ( *(instView + 4*(i) + 2) )
#define ROT_Z(i) ( *(instView + 4*(i) + 3) )

double swapd(double d)
{
    double		 a;
    unsigned char *dst = (unsigned char *)&a;
    unsigned char *src = (unsigned char *)&d;

    dst[0] = src[7];
    dst[1] = src[6];
    dst[2] = src[5];
    dst[3] = src[4];
    dst[4] = src[3];
    dst[5] = src[2];
    dst[6] = src[1];
    dst[7] = src[0];

    return a;
}

double dist(double *x, double *y, unsigned short ndim)
{
    double d;
    unsigned short i;

    d = 0;
    for(i=0; i<ndim; i++)
    {
        d += (*(x+i) - *(y+i))*(*(x+i) - *(y+i) );
    }
    d = sqrt(d);

    return d;
}

double dot(double *x,double *y, unsigned short ndim)
{
    double d;
    unsigned short i;

    d=0.0f;
    for(i=0; i<ndim; i++)
    {
        d += *(x+i) * *(y+i);
    }

    return d;
}

double tansig(double x)
{
    return 2/(1+exp(-2*x))-1;
}

Window::Window()
{
    unrecoverable = false;

    glWidget = new GLWidget;

    // settings that probably need to persist
    staticNodes = NULL;
    staticElements = NULL;
    staticNElements = 0;
    initial = NULL;

    recordingMovie = false;

    settings = NULL;

    dynamicNodes = NULL;
    dynamicElements = NULL;
    dynamicNElements = 0;

    eigen=NULL;
    means=NULL;
    range=NULL;
    scaling=NULL;

    hull=NULL;

    instP=NULL;
    instC=NULL;

    // Read the parameters

    if(!GetSettings())
    {
        unrecoverable=true;
        return;
    }

    controls.fill(0,nControls);
    controls_min.fill(0,nControls);
    controls_max.fill(0.35,nControls);

    //////////////////////////

    setCentralWidget(glWidget);

    CreateToolBars();

    SetControlsBoxes();
    SetParametersBoxes();
    SetControlsSliders();
    SetParametersSliders();

    xSlider->setValue(270 * 16);
    ySlider->setValue(0 * 16);
    zSlider->setValue(270 * 16);
    setWindowTitle(tr("Tongue Viewer (by Adam Baker)"));

    // initialize timer
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(SeekForward()));
    playing = 0;

    SetMode(M_DYNAMIC);

    UpdateTongueShape();
}

Window::~Window()
{
    TryToFree(staticNodes);
    TryToFree(staticElements);
    TryToFree(dynamicNodes);
    TryToFree(dynamicElements);
    TryToFree(eigen);
    TryToFree(means);
    TryToFree(range);
    TryToFree(scaling);
    TryToFree(hull);
    TryToFree(instP);
    TryToFree(instC);
    TryToFree(initial);
    TryToFree(instView);
}

bool Window::GetSettings()
{
    TryToFree(staticElements);
    TryToFree(dynamicNodes);
    TryToFree(dynamicElements);
    TryToFree(eigen);
    TryToFree(means);
    TryToFree(range);
    TryToFree(hull);
    TryToFree(initial);
    labControls.clear();
    labParameters.clear();

    fnSettings = QFileDialog::getOpenFileName(this,tr("Open Settings File"), "data", tr("Settings (XML) Files (*.xml)"));
    if(fnSettings==NULL || !(fnSettings.length()>0))
    {
        return false;
    }
    if(settings!=NULL)
    {
        QFileInfo fi(fnSettings);
        settings->setText("Change Settings\n("+fi.fileName()+")");
    }

    //	QString settingsXmlFilename = "C:\\QtWork\\Tongue\\release\\data\\settings.xml";
    QFile file(fnSettings);
    QXmlInputSource inputSource(&file);


    QXmlSimpleReader reader;
    ReadSettings handler;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.parse(inputSource);

    labControls = handler.controls;
    labParameters = handler.parameters;
    nControls = labControls.size();
    nParameters = labParameters.size();

    staticNElements=handler.sNEl;
    staticNNodes=handler.sNPoints;
    dynamicNElements = handler.dNEl;
    dynamicNNodes = handler.dNPoints;
    nHull=handler.nHull;

    fnStaticElements = handler.sConnectivity;
    fnDynamicElements = handler.dConnectivity;
    fnEigen = handler.dEigen;
    fnMeans = handler.dMeans;
    fnRange = handler.dRange;
    fnScaling = handler.dScaling;
    fnInitial = handler.dInitial;
    fnHull = handler.dHull;
    fnAnnPC = handler.annParametersToControls;
    fnAnnCP = handler.annControlsToParameters;

    if(!ReadStaticElements()) { qDebug() << "ReadStaticElements"; return false; }
    if(!ReadDynamicElements()) { qDebug() << "ReadDynamicElements"; return false; }
    if(!ReadEigen()) { qDebug() << "ReadEigen"; return false; }
    if(!ReadMeans()) { qDebug() << "ReadMeans"; return false; }
    if(!ReadRange()) { qDebug() << "ReadRange"; return false; }
    if(!ReadScaling()) { qDebug() << "ReadScaling"; return false; }
    if(!ReadHull()) { qDebug() << "ReadHull"; } // this is non-essential return false; }
    if(!ReadInitial()) { qDebug() << "ReadInitial"; return false; }
    if(!ReadControlToParameterANN()) { qDebug() << "ReadControlToParameterANN"; return false; }
    if(!ReadParameterToControlANN()) { qDebug() << "ReadParameterToControlANN";  return false; }

    dynamicNodes = (float*)malloc(sizeof(float)*3*dynamicNNodes);

    return true;
}

QSlider *Window::createSlider()
{
    QSlider *slider = new QSlider(Qt::Vertical);
    slider->setRange(0,360*16);
    slider->setSingleStep(16);
    slider->setPageStep(15*16);
    slider->setTickInterval(15*16);
    slider->setTickPosition(QSlider::TicksRight);
    return slider;
}

void Window::CreateToolBars()
{
    // Orientation Dock
    QDockWidget *orientationDock = new QDockWidget("Orientation",this);
    QWidget *orientationWidget = new QWidget(this);
    QGridLayout  *orientationLayout = new QGridLayout;

    xSlider = createSlider();
    ySlider = createSlider();
    zSlider = createSlider();

    rollLabel = new QLabel;
    pitchLabel = new QLabel;
    yawLabel = new QLabel;

    connect(xSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setXRotation(int)));
    connect(glWidget, SIGNAL(xRotationChanged(int)), xSlider, SLOT(setValue(int)));
    connect(ySlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setYRotation(int)));
    connect(glWidget, SIGNAL(yRotationChanged(int)), ySlider, SLOT(setValue(int)));
    connect(zSlider, SIGNAL(valueChanged(int)), glWidget, SLOT(setZRotation(int)));
    connect(glWidget, SIGNAL(zRotationChanged(int)), zSlider, SLOT(setValue(int)));

    connect(xSlider, SIGNAL(valueChanged(int)), this, SLOT(setXRotationLabel()));
    connect(ySlider, SIGNAL(valueChanged(int)), this, SLOT(setYRotationLabel()));
    connect(zSlider, SIGNAL(valueChanged(int)), this, SLOT(setZRotationLabel()));

    orientationLayout->addWidget(new QLabel(tr("Roll")),0,0);
    orientationLayout->addWidget(new QLabel(tr("Pitch")),0,1);
    orientationLayout->addWidget(new QLabel(tr("Yaw")),0,2);

    orientationLayout->addWidget(xSlider,1,0);
    orientationLayout->addWidget(ySlider,1,1);
    orientationLayout->addWidget(zSlider,1,2);

    orientationLayout->addWidget(rollLabel,2,0);
    orientationLayout->addWidget(pitchLabel,2,1);
    orientationLayout->addWidget(yawLabel,2,2);

    orientationWidget->setLayout(orientationLayout);
    orientationDock->setWidget(orientationWidget);
    orientationDock->setFloating(true);
    orientationDock->resize(100,300);
    addDockWidget(Qt::LeftDockWidgetArea,orientationDock);

    // Controls Dock
    QDockWidget *controlsDock = new QDockWidget("Controls",this);
    QWidget *controlsWidget = new QWidget(this);
    QHBoxLayout  *controlsLayout = new QHBoxLayout;
    QVector<QVBoxLayout*>  controlVLayout;
    QVector<QLabel*>  controlLabels;

    for(i=0; i<nControls; i++)
    {
        slControls << createSlider();
        leControls << new QLineEdit;
        controlVLayout << new QVBoxLayout;
        controlLabels << new QLabel(labControls[i]);
        controlVLayout.last()->addWidget(controlLabels.last());
        controlVLayout.last()->addWidget(slControls.last());
        controlVLayout.last()->addWidget(leControls.last());
        controlsLayout->addLayout(controlVLayout.last());

        connect(slControls[i], SIGNAL(sliderMoved(int)), this, SLOT(SetControlsFromSliders()));
        connect(leControls[i], SIGNAL(editingFinished()), this, SLOT(SetControlsFromBoxes()));

        connect(slControls[i], SIGNAL(sliderMoved(int)), this, SLOT(SetParametersFromControls()));
        connect(leControls[i], SIGNAL(editingFinished()), this, SLOT(SetParametersFromControls()));

        connect(slControls[i], SIGNAL(sliderMoved(int)), glWidget, SLOT(updateGL()));
        connect(leControls[i], SIGNAL(editingFinished()), glWidget, SLOT(updateGL()));
    }

    controlsWidget->setLayout(controlsLayout);
    controlsDock->setWidget(controlsWidget);
    controlsDock->setFloating(true);
    controlsDock->resize(100,300);
    addDockWidget(Qt::LeftDockWidgetArea,controlsDock);

    // Parameters Dock
    QDockWidget *parametersDock = new QDockWidget("Parameters",this);
    QWidget *parametersWidget = new QWidget(this);
    QHBoxLayout  *parametersLayout = new QHBoxLayout;
    QVector<QVBoxLayout*>  parametersVLayout;
    QVector<QLabel*>  parameterLabels;

    for(i=0; i<nParameters; i++)
    {
        slParameters << createSlider();
        leParameters << new QLineEdit;
        parametersVLayout << new QVBoxLayout;
        parameterLabels << new QLabel(labParameters[i]);
        parametersVLayout.last()->addWidget(parameterLabels.last());
        parametersVLayout.last()->addWidget(slParameters.last());
        parametersVLayout.last()->addWidget(leParameters.last());
        parametersLayout->addLayout(parametersVLayout.last());

        connect(slParameters[i], SIGNAL(sliderMoved(int)), this, SLOT(SetParametersFromSliders()));
        connect(leParameters[i], SIGNAL(editingFinished()), this, SLOT(SetParametersFromBoxes()));

        connect(slParameters[i], SIGNAL(sliderMoved(int)), this, SLOT(SetControlsFromParameters()));
        connect(leParameters[i], SIGNAL(editingFinished()), this, SLOT(SetControlsFromParameters()));

        connect(slParameters[i], SIGNAL(sliderMoved(int)), glWidget, SLOT(updateGL()));
        connect(leParameters[i], SIGNAL(editingFinished()), glWidget, SLOT(updateGL()));
    }

    parametersWidget->setLayout(parametersLayout);
    parametersDock->setWidget(parametersWidget);
    parametersDock->setFloating(true);
    parametersDock->resize(100,300);
    addDockWidget(Qt::LeftDockWidgetArea,parametersDock);

    // Animation Dock
    QDockWidget *animationDock = new QDockWidget("Animation",this);
    animationWidget = new QWidget(this);
    QVBoxLayout  *animationLayout = new QVBoxLayout;
    QHBoxLayout  *animationIconLayout = new QHBoxLayout;
    QPushButton *beginning, *end, *next, *previous;

    play = new QPushButton;
    beginning = new QPushButton;
    end = new QPushButton;
    next = new QPushButton;
    previous = new QPushButton;
    play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    beginning->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    end->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    next->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    previous->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));

    animationIconLayout->addWidget(beginning);
    animationIconLayout->addWidget(previous);
    animationIconLayout->addWidget(play);
    animationIconLayout->addWidget(next);
    animationIconLayout->addWidget(end);

    connect(beginning, SIGNAL(clicked(bool)), this, SLOT(SkipBackward()));
    connect(end, SIGNAL(clicked(bool)), this, SLOT(SkipForward()));
    connect(previous, SIGNAL(clicked(bool)), this, SLOT(SeekBackward()));
    connect(next, SIGNAL(clicked(bool)), this, SLOT(SeekForward()));
    connect(play, SIGNAL(clicked(bool)), this, SLOT(Play()));

    slProgress = new QSlider(Qt::Horizontal);
    connect(slProgress, SIGNAL(sliderMoved(int)), this, SLOT(SetAnimationFromSlider()));

    createMpegButton = new QPushButton(tr("Create MPEG"));
    connect(createMpegButton,SIGNAL(clicked()),this,SLOT(createMpeg()));

    animationLayout->addLayout(animationIconLayout);
    animationLayout->addWidget(slProgress);
    animationLayout->addWidget(createMpegButton);


    animationWidget->setLayout(animationLayout);
    animationDock->setWidget(animationWidget);
    animationDock->setFloating(true);
    addDockWidget(Qt::LeftDockWidgetArea,animationDock);

    // Settings & Info Dock
    QDockWidget *settingsDock = new QDockWidget("Settings & Information",this);
    QWidget *settingsWidget = new QWidget(this);
    QVBoxLayout  *settingsLayout = new QVBoxLayout;
    QPushButton *takePictures, *dumpBinary, *resetParameters;
    QLabel *whichSettings;


    QFileInfo fi(fnSettings);
    whichSettings = new QLabel(fi.fileName());

    hullLabel = new QLabel("<font color=grey>No hull</font>");
    settings = new QPushButton("Change Settings\n("+fnSettings+")");
    scoreParameter = new QPushButton("Open Parameter Score\n(no score open)");
    scoreControl = new QPushButton("Open Control Score\n(no score open)");
    tongue = new QPushButton("Open Static\n(no file open)");
    resetParameters = new QPushButton("Reset Parameters");

    connect(settings, SIGNAL(clicked(bool)), this, SLOT(GetSettings()));
    connect(scoreParameter, SIGNAL(clicked(bool)), this, SLOT(OpenParameterScore()));
    connect(scoreControl, SIGNAL(clicked(bool)), this, SLOT(OpenControlScore()));
    connect(tongue, SIGNAL(clicked(bool)), this, SLOT(OpenStatic()));
    connect(resetParameters, SIGNAL(clicked(bool)), this, SLOT(ResetParameters()));


    modeCombo = new QComboBox;
    modeCombo->addItem("Dynamic (using the sliders)");
    modeCombo->addItem("Static (files)");
    modeCombo->addItem("Parameter Animation");
    modeCombo->addItem("Control Animation");
    connect(modeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(SetModeFromCombo()));

    doHull = new QCheckBox("Check that coordinate is within\nthe hull");
    doSynchrony = new QCheckBox("Keep control activations synchronized\nwith parameters with a neural net");

    takePictures = new QPushButton("Take Pictures");
    dumpBinary = new QPushButton("Dump Binary");

    connect(takePictures, SIGNAL(clicked(bool)), glWidget, SLOT(TakePictures()));
    connect(dumpBinary, SIGNAL(clicked(bool)), this, SLOT(DumpBinary()));

    settingsLayout->addWidget(whichSettings);
    settingsLayout->addWidget(modeCombo);
    settingsLayout->addWidget(scoreParameter);
    settingsLayout->addWidget(scoreControl);
    settingsLayout->addWidget(tongue);
    settingsLayout->addWidget(hullLabel);
    settingsLayout->addWidget(doHull);
    settingsLayout->addWidget(doSynchrony);
    settingsLayout->addWidget(resetParameters);
    settingsLayout->addWidget(takePictures);
    settingsLayout->addWidget(dumpBinary);

    settingsWidget->setLayout(settingsLayout);
    settingsDock->setWidget(settingsWidget);
    settingsDock->setFloating(true);
    addDockWidget(Qt::LeftDockWidgetArea,settingsDock);

    if(hull == NULL)
    {
        doHull->setChecked(false);
        doHull->setEnabled(false);
    }

    if(wCP==NULL || bCP==NULL)
    {
        doSynchrony->setChecked(false);
        doSynchrony->setEnabled(false);
    }

    // initialize rotation sliders
    setXRotationLabel();
    setYRotationLabel();
    setZRotationLabel();
}

void Window::SetControlsFromParameters()
{
    if(wCP==NULL || bCP==NULL || !doSynchrony->isChecked())
    {
        return;
    }

    for(i=0; i<nControls; i++)
    {
        *(inputCP+i) = 2*(parameters[i] -IN_MIN_CP(i))/(IN_MAX_CP(i)-IN_MIN_CP(i)) - 1;
    }

    for(i=0; i< *(nCP+0); i++) // first layer
    {
        OUTPUT_CP(0,i) = tansig( dot(inputCP, *(wCP+0) + nControls*i  ,nControls) + BIAS_CP(0,i) );
    }

    for(k=1; k<nlCP; k++) // subsequent layer
    {
        for(i=0; i< *(nCP+k); i++)
        {
            OUTPUT_CP(k,i) = tansig( dot( OUTPUT_ADDR_CP(k-1) , LW_ROW_CP(k,i), *(nCP+k-1)) + BIAS_CP(k,i) );
        }
    }

    for(i=0; i<*(nCP+nlCP-1); i++)
    {
        controls[i] = ((OUTPUT_CP(nlCP-1,i)+1)/2)*(OUT_MAX_CP(i)-OUT_MIN_CP(i))+OUT_MIN_CP(i);
    }

    SetControlsBoxes();
    SetControlsSliders();
}

void Window::SetParametersFromControls()
{
    if(wPC==NULL || bPC==NULL)
    {
        QString string = "NULL"; qCritical(string.toUtf8());
        return;
    }

    for(i=0; i<nControls; i++)
    {
        *(inputPC+i) = 2*(controls[i] -IN_MIN_PC(i))/(IN_MAX_PC(i)-IN_MIN_PC(i)) - 1;
    }

    for(i=0; i< *(nPC+0); i++) // first layer
    {
        OUTPUT_PC(0,i) = tansig( dot(inputPC, *(wPC+0) + nControls*i  ,nControls) + BIAS_PC(0,i) );
    }

    for(k=1; k<nlPC; k++) // subsequent layer
    {
        for(i=0; i< *(nPC+k); i++)
        {
            OUTPUT_PC(k,i) = tansig( dot( OUTPUT_ADDR_PC(k-1) , LW_ROW_PC(k,i), *(nPC+k-1)) + BIAS_PC(k,i) );
        }
    }

    for(i=0; i<*(nPC+nlPC-1); i++)
    {
        parameters[i] = ((OUTPUT_PC(nlPC-1,i)+1)/2)*(OUT_MAX_PC(i)-OUT_MIN_PC(i))+OUT_MIN_PC(i);
    }

    SetParametersBoxes();
    SetParametersSliders();

    UpdateTongueShape();
}


void Window::SetParametersFromSliders()
{
    SetMode(M_DYNAMIC);
    for(i=0;i<nParameters;i++)
    {
        parameters[i] = ((double)slParameters[i]->sliderPosition()/5760.0f) * (parameters_max[i]-parameters_min[i]) + parameters_min[i];
    }
    SetParametersBoxes();

    UpdateTongueShape();
}


void Window::SetParametersFromBoxes()
{
    SetMode(M_DYNAMIC);
    for(i=0;i<nParameters;i++)
    {
        parameters[i] = leParameters[i]->text().toDouble();
        slParameters[i]->setSliderPosition( (int)(((parameters[i]-parameters_min[i])/(parameters_max[i]-parameters_min[i]))*5760) );
    }
    SetParametersSliders();

    UpdateTongueShape();
}

void Window::SetParametersBoxes()
{
    QString str;
    for(i=0;i<nParameters;i++)
    {
        str.setNum(parameters[i],'g',4);
        leParameters[i]->setText(str);
    }
}

void Window::SetParametersSliders()
{
    for(i=0;i<nParameters;i++)
    {
        slParameters[i]->setSliderPosition( (int)(((parameters[i]-parameters_min[i])/(parameters_max[i]-parameters_min[i]))*5760) );
    }
}

void Window::SetControlsFromSliders()
{
    SetMode(M_DYNAMIC);
    for(i=0;i<nControls;i++)
    {
        controls[i] = ((double)slControls[i]->sliderPosition()/5760.0f) * (controls_max[i]-controls_min[i]) + controls_min[i];
    }
    SetControlsBoxes();
}


void Window::SetControlsFromBoxes()
{
    SetMode(M_DYNAMIC);
    for(i=0;i<nControls;i++)
    {
        controls[i] = leControls[i]->text().toDouble();
    }
    SetControlsSliders();
}

void Window::SetControlsSliders()
{
    for(i=0;i<nControls;i++)
    {
        slControls[i]->setSliderPosition( (int)(((controls[i]-controls_min[i])/(double)(controls_max[i]-controls_min[i]))*5760) );
    }
}

void Window::SetControlsBoxes()
{
    QString str;
    for(i=0;i<nControls;i++)
    {
        str.setNum(controls[i],'g',4);
        leControls[i]->setText(str);
    }
}

void Window::CheckHull()
{
    double *test;
    quint32 i;
    quint8 result = 1;

    if(hull==NULL || !doHull->isChecked())
    {
        hullLabel->setText("");
        return;
    }

    test = (double*)malloc(sizeof(double)*nParameters);

    for(i=0; i<nParameters; i++)
    {
        *(test+i) = parameters[i];
    }

    for(i=0; i<nHull; i++)
    {
        if( dot(test,hull+i*nParameters,nParameters) > 1.0001) // 1 plus 0.0001 for rounding
        {
            result = 0;
        }
    }
    free(test);

    if(result)
    {
        hullLabel->setText("<font color=green>Within hull</font>");
    }
    else
    {
        hullLabel->setText("<font color=red>Outside hull</font>");
    }
}

void Window::ChangeSettings()
{
}

void Window::OpenStatic()
{
    fnStatic = QFileDialog::getOpenFileName(this,tr("Open Static Tongue Posture"), "data", tr("Tongue Posture Binary Files (*.bin)"));

    if(ReadStatic())
    {
        QFileInfo info(fnStatic);
        tongue->setText("Open Static\n("+ info.fileName()+")");

        SetMode(M_STATIC);
    }
}

void Window::OpenParameterScore()
{
    fnAnimationP = QFileDialog::getOpenFileName(this,tr("Open Parameter Score"), "data", tr("Score Files (*.txt)"));
    if(ReadAnimationP())
    {
        QFileInfo info(fnAnimationP);
        scoreParameter->setText("Open Parameter Score\n("+ info.fileName()+")");
    }
}

void Window::OpenControlScore()
{
    fnAnimationC = QFileDialog::getOpenFileName(this,tr("Open Control Score"), "data", tr("Score Files (*.txt)"));
    if(ReadAnimationC())
    {
        QFileInfo info(fnAnimationC);
        scoreControl->setText("Open Control Score\n("+ info.fileName()+")");
    }
}

bool Window::ReadStatic()
{
    if(staticNodes==NULL)
    {
        staticNodes = (float*)malloc(sizeof(float)*staticNNodes*3);
    }

    QFile file(fnStatic);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnStatic + "\n(fnStatic)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for(i=0; i<staticNNodes*3; i++)
    {
        in >> *(staticNodes+i);
    }
    file.close();

    return true;
}

void Window::SetModeFromCombo()
{
    SetMode(modeCombo->currentIndex());
}

void Window::SetMode(unsigned char m)
{
    if(timer != NULL && timer->isActive()) { timer->stop(); }

    mode = m;
    modeCombo->setCurrentIndex(mode);

    switch(mode)
    {
    case M_DYNAMIC:
    case M_PSCORE:
    case M_CSCORE:
        glWidget->SetVariables(dynamicNElements,dynamicElements,dynamicNodes);
        break;
    case M_STATIC:
        glWidget->SetVariables(staticNElements,staticElements,staticNodes);
        break;
    }

    if(mode == M_PSCORE || mode == M_CSCORE)
        animationWidget->setEnabled(true);
    else
        animationWidget->setEnabled(false);

    glWidget->updateGL();
}

void Window::TryToFree(float *var)
{
    if(var!=NULL) { free(var); var=NULL; }
}

void Window::TryToFree(double *var)
{
    if(var!=NULL) { free(var); var=NULL; }
}

void Window::TryToFree(quint16 *var)
{
    if(var!=NULL) { free(var); var=NULL; }
}

void Window::TryToFree(quint32 *var)
{
    if(var!=NULL) { free(var); var=NULL; }
}

void Window::UpdateTongueShape()
{
    if(dynamicNodes==NULL || dynamicElements==NULL || eigen==NULL || means==NULL)
        return;


    // calculate the vertex positions
    for(i=0; i<(3*dynamicNNodes); i++)
    {
        *(dynamicNodes+i) = 0.0f;
        for(j=0; j<nParameters; j++)
            *(dynamicNodes+i) = *(dynamicNodes+i) + *(eigen + j*(3*dynamicNNodes) + i) * parameters[j];

        *(dynamicNodes+i) = *(dynamicNodes+i) * *(scaling+i) + *(means+i);
    }

    for(i=0; i<10; i++)
        qDebug() << *(dynamicNodes+i);

    CheckHull();

    glWidget->updateGL();
}


bool Window::ReadDynamicElements()
{
    if(dynamicElements==NULL)
    {
        dynamicElements = (quint16*)malloc(sizeof(quint16)*dynamicNElements*3);
    }

    QFile file(fnDynamicElements);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnDynamicElements + "\n(fnDynamicElements)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for(i=0; i<dynamicNElements*3; i++)
    {
        in >> *(dynamicElements+i);
    }
    file.close();

    return true;
}

bool Window::ReadStaticElements()
{
    if(staticElements==NULL)
    {
        staticElements = (quint16*)malloc(sizeof(quint16)*staticNElements*3);
    }

    QFile file(fnStaticElements);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnStaticElements + "\n(fnStaticElements)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::SinglePrecision);
    for(i=0; i<staticNElements*3; i++)
    {
        in >> *(staticElements+i);
    }
    file.close();

    return true;
}

bool Window::ReadEigen()
{
    if(eigen==NULL)
    {
        eigen = (double*)malloc(sizeof(double)*3*dynamicNNodes*3*dynamicNNodes);
    }

    QFile file(fnEigen);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnEigen + "\n(fnEigen)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);
    for(i=0; i<3*dynamicNNodes*3*dynamicNNodes; i++)
    {
        in >> *(eigen+i);
    }
    file.close();

    return true;
}

bool Window::ReadMeans()
{
    if(means==NULL)
    {
        means = (double*)malloc(sizeof(double)*3*dynamicNNodes);
    }

    QFile file(fnMeans);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnMeans + "\n(fnMeans)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);
    for(i=0; i<dynamicNNodes*3; i++)
    {
        in >> *(means+i);
    }
    file.close();

    return true;
}

bool Window::ReadScaling()
{
    if(scaling==NULL)
    {
        scaling = (double*)malloc(sizeof(double)*3*dynamicNNodes);
    }

    if(fnScaling == "")
    {
        //	qDebug() << "Filling with ones...";
        for(i=0; i<dynamicNNodes*3; i++)
        {
            //	    qDebug() << i;
            *(scaling+i) = 1;
        }
        //	qDebug() << "Done filling";
        return true;
    }

    QFile file(fnScaling);
    if( file.open(QIODevice::ReadOnly) == false ) { qDebug() << "Error opening " + fnScaling + "\n(fnScaling)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);
    for(i=0; i<dynamicNNodes*3; i++)
    {
        in >> *(scaling+i);
    }
    file.close();

    return true;
}

bool Window::ReadInitial()
{
    if(initial==NULL)
    {
        initial = (double*)malloc(sizeof(double)*3*dynamicNNodes);
    }

    QFile file(fnInitial);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnInitial + "\n(fnInitial)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);
    for(i=0; i<dynamicNNodes*3; i++)
    {
        in >> *(initial+i);
    }
    file.close();

    InitializeParameters();

    return true;
}


void Window::InitializeParameters()
{
    parameters.resize(nParameters);
    for(i=0; i<nParameters; i++)
    {
        parameters[i] = *(initial+i);
    }
}

void Window::ResetParameters()
{
    parameters.resize(nParameters);
    for(i=0; i<nParameters; i++)
    {
        parameters[i] = *(initial+i);
    }
    SetParametersSliders();
    UpdateTongueShape();
}

bool Window::ReadRange()
{
    if(range==NULL)
    {
        range = (double*)malloc(sizeof(double)*2*3*dynamicNNodes);
    }

    QFile file(fnRange);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnRange + "\n(fnRange)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);
    for(i=0; i<2*3*dynamicNNodes; i++)
    {
        in >> *(range+i);
    }
    file.close();


    parameters_min.resize(nParameters);
    for(i=0; i<nParameters; i++)
        parameters_min[i] = *(range+i);

    parameters_max.resize(nParameters);
    for(i=0; i<nParameters; i++)
        parameters_max[i] = *(range+3*dynamicNNodes+i);

    return true;
}

bool Window::ReadHull()
{
    QFile file(fnHull);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnHull + "\n(fnHull)"; return false; }

    if(hull==NULL)
        hull = (double*)malloc(sizeof(double)*nHull*nParameters);

    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);
    for(i=0; i<nHull*nParameters; i++)
        in >> *(hull+i);
    file.close();

    return true;
}

bool Window::ReadControlToParameterANN()
{
    //	fnAnnPC = fn;
    if(fnAnnPC.length()==0) { return true; }

    QFile file(fnAnnPC);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnAnnPC + "\n(fnAnnPC)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);

    in >> nlPC >> nControls;

    nPC = (quint32*)malloc(nlPC*sizeof(quint32));

    for(i=0; i<nlPC; i++)
        in >> *(nPC+i);

    inputPC = (double*)malloc(nControls*sizeof(double));
    inminPC = (double*)malloc(nControls*sizeof(double));
    inmaxPC = (double*)malloc(nControls*sizeof(double));
    outminPC = (double*)malloc(*(nPC+nlPC-1)*sizeof(double));
    outmaxPC = (double*)malloc(*(nPC+nlPC-1)*sizeof(double));
    wPC = (double **)malloc(nlPC*sizeof(double *));

    *(wPC+0) = (double*)malloc( *(nPC+0) * nControls * sizeof(double) );
    for(i=0; i< *(nPC+0) * nControls; i++)
        in >> *(*(wPC+0)+i);

    for(i=1; i<nlPC; i++)
    {
        *(wPC+i) = (double*)malloc( *(nPC+i) * *(nPC+i-1) * sizeof(double) );
        for(j=0; j< *(nPC+i) * *(nPC+i-1); j++ )
            in >> *(*(wPC+i)+j);
    }

    bPC = (double **)malloc(nlPC*sizeof(double *));
    for(i=0; i<nlPC; i++)
    {
        *(bPC+i) = (double*)malloc( *(nPC+i) * sizeof(double) );
        for(j=0; j< *(nPC+i); j++ )
            in >> *(*(bPC+i)+j);

    }

    for(i=0; i<nControls; i++)
        in >> *(inminPC+i);

    for(i=0; i<nControls; i++)
        in >> *(inmaxPC+i);

    for(i=0; i<*(nPC+nlPC-1); i++)
        in >> *(outminPC+i);

    for(i=0; i<*(nPC+nlPC-1); i++)
        in >> *(outmaxPC+i);

    outputPC = (double **)malloc(nlPC*sizeof(double *));
    for(i=0; i<nlPC; i++)
        *(outputPC+i) = (double*)malloc( *(nPC+i) * sizeof(double) );

    file.close();

    return true;
}

bool Window::ReadParameterToControlANN()
{
    //	fnAnnCP = fn;
    if(fnAnnCP.length()==0) { return true; }

    quint32 dummy;
    QFile file(fnAnnCP);
    if( file.open(QIODevice::ReadOnly) == false ) { qCritical() << "Error opening " + fnAnnCP  + "\n(fnAnnCP)"; return false; }
    QDataStream in(&file);
    in.setByteOrder(QDataStream::LittleEndian);
    in.setFloatingPointPrecision(QDataStream::DoublePrecision);

    in >> nlCP >> dummy;

    if(nParameters!=dummy)
    {
        QString string = "The specified network is not compatible with the number of parameters you\'re using."; qCritical(string.toUtf8());
        return false;
    }

    nCP = (quint32*)malloc(nlCP*sizeof(quint32));

    for(i=0; i<nlCP; i++)
        in >> *(nCP+i);


    inputCP = (double*)malloc(nParameters*sizeof(double));
    inminCP = (double*)malloc(nParameters*sizeof(double));
    inmaxCP = (double*)malloc(nParameters*sizeof(double));
    outminCP = (double*)malloc(*(nCP+nlCP-1)*sizeof(double));
    outmaxCP = (double*)malloc(*(nCP+nlCP-1)*sizeof(double));
    wCP = (double **)malloc(nlCP*sizeof(double *));
    *(wCP+0) = (double*)malloc( *(nCP+0) * nParameters * sizeof(double) );

    for(i=0; i< *(nCP+0) * nParameters; i++)
    {
        in >> *(*(wCP+0)+i);
    }

    for(i=1; i<nlCP; i++)
    {
        *(wCP+i) = (double*)malloc( *(nCP+i) * *(nCP+i-1) * sizeof(double) );
        for(j=0; j< *(nCP+i) * *(nCP+i-1); j++)
        {
            in >> *(*(wCP+i)+j);
        }
    }

    bCP = (double **)malloc(nlCP*sizeof(double *));
    for(i=0; i<nlCP; i++)
    {
        *(bCP+i) = (double*)malloc( *(nCP+i) * sizeof(double) );
        for(j=0; j< *(nCP+i); j++)
        {
            in >> *(*(bCP+i)+j);
        }
    }

    for(i=0; i<nParameters; i++)
    {
        in >> *(inminCP+i);
    }

    for(i=0; i<nParameters; i++)
    {
        in >> *(inmaxCP+i);
    }

    for(i=0; i< *(nCP+nlCP-1); i++)
    {
        in >> *(outminCP+i);
    }

    for(i=0; i< *(nCP+nlCP-1); i++)
    {
        in >> *(outmaxCP+i);
    }

    outputCP = (double **)malloc(nlCP*sizeof(double *));
    for(i=0; i<nlCP; i++)
    {
        *(outputCP+i) = (double*)malloc( *(nCP+i) * sizeof(double) );
    }

    file.close();

    return true;
}

bool Window::ReadAnimationP()
{
    SetMode(M_PSCORE);

    quint32 nPara;
    QByteArray ba;
    ba = fnAnimationP.toLatin1();

    FILE *fid;
    fid = fopen(ba.data(),"r");
    if(fid==NULL)
    {
        QString string = "Could not open: " + fnHull + "\n(Window::ReadAnimationP())"; qCritical(string.toUtf8());
        return false;
    }

    fscanf(fid,"%lu\n",(unsigned long*)&nInstP);
    fscanf(fid,"%lu\n",(unsigned long*)&nPara);

    if(nPara!=nParameters)
    {
        QString string = "The number of parameters specified in the animation ("+QString::number(nPara)+") is not the same as the number of parameters in your XML settings file ("+QString::number(nParameters)+")."; qCritical(string.toUtf8());
        return false;
    }

    fscanf(fid,"%lg\n",&fpsP);
    TryToFree(instP);
    instP = (double*)malloc(sizeof(double)*nInstP*(1+nParameters));

    for(i=0; i<nInstP; i++)
    {
        fscanf(fid,"%lg\t",instP+(1+nParameters)*i); // read the time
        for(j=1; j<=nParameters; j++)
        {
            fscanf(fid,"%lg\t",instP+(1+nParameters)*i+j); // read a coordinate
        }

    }

    ReadView(fid);

    frameP=-1;
    inP=0;

    lastFrameP = (quint32)floor( TIME_P(nInstP-1) / (1.0f/fpsP)) ;

    slProgress->setRange(0,lastFrameP);

    fclose(fid);

    return true;
}

void Window::ReadView(FILE *fid)
{
    char *test = (char*)malloc(5);
    fgets(test,5,fid);
    if( strcmp( test, "view" ) == 0 )
    {
        fscanf(fid,"%lu\n",(unsigned long*)&nInstView);

        instView = (double*)malloc(sizeof(double)*(30)*4);

        for(i=0; i<nInstView; i++)
            fscanf(fid,"%lg\t%lg\t%lg\t%lg\n",instView + 4*i + 0,instView + 4*i + 1,instView + 4*i + 2,instView + 4*i + 3);
    }
    else
    {
        free(test);
        instView = 0;
        return;
    }
    free(test);

    qDebug() << nInstView;
    for(i=0; i<nInstView; i++)
    {
        qDebug() << VIEW_TIME(i) << ROT_X(i) << ROT_Y(i) << ROT_Z(i);
    }
}

bool Window::ReadAnimationC()
{
    SetMode(M_CSCORE);

    quint32 nPara;
    QByteArray ba;
    ba = fnAnimationC.toLatin1();

    FILE *fid;
    fid = fopen(ba.data(),"r");
    if(fid==NULL)
    {
        QString string = "Could not open: " + fnHull + "\n(Window::ReadAnimationC())"; qCritical(string.toUtf8());
        return false;
    }

    fscanf(fid,"%lu\n",(unsigned long*)&nInstC);
    fscanf(fid,"%lu\n",(unsigned long*)&nPara);

    if(nPara!=nControls)
    {
        QString string = "This file is not compatible with the current settings."; qCritical(string.toUtf8());
        return false;
    }

    fscanf(fid,"%lg\n",&fpsC);
    TryToFree(instC);
    instC = (double*)malloc(sizeof(double)*nInstC*(1+nControls));

    for(i=0; i<nInstC; i++)
    {
        fscanf(fid,"%lg\t",instC+(1+nControls)*i); // read the time
        for(j=1; j<=nControls; j++)
        {
            fscanf(fid,"%lg\t",instC+(1+nParameters)*i+j); // read a coordinate
        }

    }

    ReadView(fid);

    frameC=-1;
    inC=0;

    lastFrameC = (quint32)floor( TIME_C(nInstC-1) / (1.0f/fpsC)) ;

    slProgress->setRange(0,lastFrameC);

    fclose(fid);

    return true;
}


void Window::SkipBackward()
{
    switch(mode)
    {
    case M_PSCORE:
        frameP = 0;
        if(playing) { Play(); }
        break;
    case M_CSCORE:
        frameC = 0;
        if(playing) { Play(); }
        break;
    }
    UpdateAnimation();
}

void Window::SkipForward()
{
    switch(mode)
    {
    case M_PSCORE:
        frameP = lastFrameP;
        if(playing) { Play(); }
        break;
    case M_CSCORE:
        frameC = lastFrameC;
        if(playing) { Play(); }
        break;
    }
    UpdateAnimation();
}

void Window::SeekBackward()
{
    switch(mode)
    {
    case M_PSCORE:
        frameP--;
        break;
    case M_CSCORE:
        frameC--;
        break;
    }
    UpdateAnimation();
}

void Window::SeekForward()
{
    switch(mode)
    {
    case M_PSCORE:
        frameP++;
        break;
    case M_CSCORE:
        frameC++;
        break;
    }
    UpdateAnimation();
}

void Window::Play()
{
    switch(mode)
    {
    case M_PSCORE:
        if(playing)
        {
            playing=false;
            timer->stop();
            play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        }
        else
        {
            playing=true;
            timer->start((int)(1000.0f/fpsP));
            play->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        }
        break;
    case M_CSCORE:
        if(playing)
        {
            playing=false;
            timer->stop();
            play->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        }
        else
        {
            playing=true;
            timer->start((int)(1000.0f/fpsC));
            play->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
        }
        break;
    }
}

void Window::UpdateAnimation()
{
    double time=0;
    int frame=0;

    switch(mode)
    {
    case M_PSCORE:
        if(frameP<0 || frameP > lastFrameP)
        {
            frameP=0;
            if(recordingMovie)
            {
                recordingMovie = false;
                playing = false;
                timer->stop();
                saveMovieFile();
                return;
            }
        }
        time = (1.0f/fpsP)*frameP + TIME_P(0);
        frame = frameP;

        inP = 0;
        while( time > TIME_P(inP+1) )
            inP++;

        for(i=0; i<nParameters; i++)
            parameters[i] = LEVEL_P(inP,i) + (time-TIME_P(inP)) * (LEVEL_P(inP+1,i)-LEVEL_P(inP,i))/(TIME_P(inP+1)-TIME_P(inP));


        slProgress->setValue(frameP);

        UpdateTongueShape();
        SetParametersSliders();
        SetControlsFromParameters();

        break;
    case M_CSCORE:
        if(frameC<0 || frameC > lastFrameC)
        {
            frameC=0;
            if(recordingMovie)
            {
                recordingMovie = false;
                playing = false;
                timer->stop();
                saveMovieFile();
                return;
            }
        }
        time = (1.0f/fpsC)*frameC + TIME_C(0);
        frame = frameC;

        inC = 0;
        while( time > TIME_C(inC+1) )
            inC++;

        for(i=0; i<nControls; i++)
            controls[i] = LEVEL_C(inC,i) + (time-TIME_C(inC)) * (LEVEL_C(inC+1,i)-LEVEL_C(inC,i))/(TIME_C(inC+1)-TIME_C(inC));

        slProgress->setValue(frameC);

        SetControlsSliders();
        SetParametersFromControls();
        break;
    }

    // update the view settings, if applicable
    if(instView != 0)
    {
        quint32 inT = 0;

        double xrot, yrot, zrot;
        //	qDebug() << time << VIEW_TIME(nInstView-1);
        if( time > VIEW_TIME(nInstView-1) )
        {
            xrot = ROT_X(nInstView-1);
            yrot = ROT_Y(nInstView-1);
            zrot = ROT_Z(nInstView-1);
        }
        else
        {
            while( time > VIEW_TIME(inT+1) )
                inT++;

            // x-rotation
            xrot = ROT_X(inT) + (time-VIEW_TIME(inT)) * (ROT_X(inT+1)-ROT_X(inT))/(VIEW_TIME(inT+1)-VIEW_TIME(inT));

            // z-rotation
            yrot = ROT_Y(inT) + (time-VIEW_TIME(inT)) * (ROT_Y(inT+1)-ROT_Y(inT))/(VIEW_TIME(inT+1)-VIEW_TIME(inT));

            // z-rotation
            zrot = ROT_Z(inT) + (time-VIEW_TIME(inT)) * (ROT_Z(inT+1)-ROT_Z(inT))/(VIEW_TIME(inT+1)-VIEW_TIME(inT));
        }
        while(zrot < 0)
            zrot += 360;

        xSlider->setValue(16*(int)xrot);
        ySlider->setValue(16*(int)yrot);
        zSlider->setValue(16*(int)zrot);
    }

    if(recordingMovie)
    {
        timer->stop();
        QString fn;
        fn.sprintf("tmp/frame%.6d.png",frame);
        glWidget->saveView(fn);
        timer->start();
    }
}

void Window::SetAnimationFromSlider()
{
    switch(mode)
    {
    case M_PSCORE:
        frameP = slProgress->value();
        break;
    case M_CSCORE:
        frameC = slProgress->value();
        break;
    }
    UpdateAnimation();
}


void Window::DumpBinary()
{
    QByteArray ba;
    QString fn;
    FILE *fid;
    char *buffer = (char*)malloc(1024);
    QString string;

    switch(mode)
    {
    case M_STATIC:
        string = "It doesn't make sense to do this in static mode, since you already have a binary file, "+fnStatic+"."; qCritical(string.toUtf8());
        break;
    case M_DYNAMIC:
        if(dynamicNodes==NULL)
        {
            string = "No tongue to dump!"; qCritical(string.toUtf8());
            return;
        }

        bool ok;
        fn = QInputDialog::getText(this, tr("Enter a filename"),tr("Filename (.bin assumed):"), QLineEdit::Normal,tr(""), &ok);
        if (!ok || fn.isEmpty())
        {
            return;
        }
        fn += ".bin";

        fid = fopen(fn.toLocal8Bit().data(),"wb");

        if(fid==NULL)
        {
            string = "Couldn\'t write to: "+fn; qCritical(string.toUtf8());
            return;
        }
        fwrite(dynamicNodes,sizeof(float),3*dynamicNNodes,fid);
        fclose(fid);
        break;
    case M_PSCORE:
    case M_CSCORE:
        break;
    }

    free(buffer);
}

void Window::setXRotationLabel()
{
    int angle = xSlider->value();
    glWidget->normalizeAngle(&angle);
    angle /= 16;
    rollLabel->setNum(angle);
}

void Window::setYRotationLabel()
{
    int angle = ySlider->value();
    glWidget->normalizeAngle(&angle);
    angle /= 16;
    pitchLabel->setNum(angle);
}

void Window::setZRotationLabel()
{
    int angle = zSlider->value();
    glWidget->normalizeAngle(&angle);
    angle /= 16;
    yawLabel->setNum(angle);
}

void Window::createMpeg()
{
    if(mode == M_PSCORE || mode == M_CSCORE)
    {
        movieFilename = QFileDialog::getSaveFileName(this, tr("Save File"),
                                                     "",
                                                     tr("MPEG Movies (*.mpg)"));
        if(movieFilename == "")
            return;

        recordingMovie = true;

        frameP = -1;
        frameC = -1;

        QDir dir("tmp");
        if(!dir.exists())
            QDir::current().mkdir("tmp");
        dir = QDir::current();
        dir.cd("tmp");
        QStringList files = dir.entryList(QDir::Files);
        for(int i=0; i<files.count(); i++)
            QFile::remove( dir.absoluteFilePath(files.at(i)) );

        Play();
    }
}

void Window::saveMovieFile()
{
    double delay=6.0f;
    if(mode == M_PSCORE)
    {
        delay = 1.0f/fpsP;
    }
    else if(mode == M_CSCORE)
    {
        delay = 1.0f/fpsC;
    }

    QProcess *myProcess = new QProcess(this);
    connect(myProcess,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(cleanUpMovieFiles(int,QProcess::ExitStatus)));
    QStringList arguments;
    arguments << "-delay" << QString::number(delay) << "-quality" << "95" << "tmp/frame*png" << movieFilename;
    myProcess->start("convert", arguments);
    qDebug() << "convert " + arguments.join(" ");
}

void Window::cleanUpMovieFiles( int exitCode, QProcess::ExitStatus exitStatus )
{
    Q_UNUSED(exitStatus);
    if( exitCode == 0)
    {
        QDir dir = QDir::current();
        dir.cd("tmp");
        QStringList files = dir.entryList(QDir::Files);
        for(int i=0; i<files.count(); i++)
            QFile::remove( dir.absoluteFilePath(files.at(i)) );
        dir.rmdir( dir.absolutePath() );
    }
}
