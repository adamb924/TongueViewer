#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QProcess>

class QSlider;
class GLWidget;
class QDockWidget;
class QToolBar;
class QLineEdit;
class QHBoxLayout;
class QVBoxLayout;
class QPushButton;
class QComboBox;
class QLabel;
class QCheckBox;
class ReadSettings;
class QStringList;

class Window : public QMainWindow
{
    Q_OBJECT

public:
    Window();
    ~Window();
    bool unrecoverable;

private:
    QSlider *createSlider();
    void CreateToolBars();

    QString fnSettings;

    // application parameters
    quint8 mode;
    quint32 i, j, k;
    QVector<double> controls, parameters;
    QVector<double> controls_min, parameters_min;
    QVector<double> controls_max, parameters_max;
    QString fnStatic;

    // model parameters
    quint32 nControls, nParameters;

    // static
    float *staticNodes;
    quint16 *staticElements;
    quint32 staticNElements, staticNNodes;

    // dynamic
    float *dynamicNodes;
    double *eigen;
    double *means;
    double *range;
    double *initial;
    double *scaling;
    quint16 *dynamicElements;
    quint32 dynamicNElements, dynamicNNodes, dynamicOrigDimen;
    QString fnStaticElements, fnDynamicElements, fnEigen, fnMeans, fnRange, fnHull, fnAnnPC, fnAnnCP, fnInitial, fnScaling;

    // hull
    quint32 nHull;
    double *hull;

    // ANN data
    // Parameters from Controls
    quint32 nlPC, *nPC;
    double **wPC, **bPC, **outputPC, *inputPC, *outminPC, *outmaxPC, *inminPC, *inmaxPC;

    // Controls from Parameters
    quint32 nlCP, *nCP;
    double **wCP, **bCP, **outputCP, *inputCP, *outminCP, *outmaxCP, *inminCP, *inmaxCP;

    // Animation
    QTimer *timer;
    bool playing;

    // recording
    bool recordingMovie;
    QString movieFilename;

    // Parameter Animation
    QString fnAnimationP;
    quint32 nInstP;
    double *instP;
    double *instView;
    quint32 nInstView;
    quint32 inP;
    qint32 frameP, lastFrameP;
    double fpsP;

    // Control Animation
    QString fnAnimationC;
    quint32 nInstC;
    double *instC;
    quint32 inC;
    qint32 frameC, lastFrameC;
    double fpsC;
    QWidget *animationWidget;

    void UpdateAnimation();

    // File Reading

    bool ReadStatic();
    bool ReadStaticElements();
    bool ReadDynamicElements();
    bool ReadEigen();
    bool ReadMeans();
    bool ReadScaling();
    bool ReadRange();
    bool ReadHull();
    bool ReadInitial();

    bool ReadControlToParameterANN();
    bool ReadParameterToControlANN();

    bool ReadAnimationP();
    bool ReadAnimationC();
    void ReadView(FILE *fid);

    QLabel *rollLabel;
    QLabel *pitchLabel;
    QLabel *yawLabel;

    // widgets
    QVector<QSlider*> slParameters;
    QVector<QLineEdit*> leParameters;
    QStringList labParameters;

    QVector<QSlider*> slControls;
    QVector<QLineEdit*> leControls;
    QStringList labControls;

    QPushButton *scoreParameter, *scoreControl, *tongue, *play;
    QComboBox *modeCombo;

    QCheckBox *doHull;
    QCheckBox *doSynchrony;

    QLabel *hullLabel;

    QPushButton *settings;

    QPushButton *createMpegButton;

    QSlider *slProgress;

    GLWidget *glWidget;
    QSlider *xSlider;
    QSlider *ySlider;
    QSlider *zSlider;

    void TryToFree(float *var);
    void TryToFree(quint16 *var);
    void TryToFree(quint32 *var);
    void TryToFree(double *var);

private slots:
    void createMpeg();
    void saveMovieFile();
    void cleanUpMovieFiles( int exitCode, QProcess::ExitStatus exitStatus );

    void setXRotationLabel();
    void setYRotationLabel();
    void setZRotationLabel();

    void SetControlsFromParameters();
    void SetParametersFromControls();
    void SetParametersFromSliders();
    void SetParametersFromBoxes();
    void SetControlsFromSliders();
    void SetControlsFromBoxes();
    void SetControlsSliders();
    void SetControlsBoxes();
    void SetParametersSliders();
    void SetParametersBoxes();
    void CheckHull();
    void ChangeSettings();
    void OpenStatic();
    void OpenParameterScore();
    void OpenControlScore();
    void SetMode(unsigned char m);
    void SetModeFromCombo();
    void UpdateTongueShape();
    bool GetSettings();
    void InitializeParameters();
    void ResetParameters();

    void SkipBackward();
    void SkipForward();
    void SeekBackward();
    void SeekForward();
    void Play();
    void SetAnimationFromSlider();
    void DumpBinary();
};

#endif
