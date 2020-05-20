#ifndef RIGID_BODY_SIMULATOR_H
#define RIGID_BODY_SIMULATOR_H

#include <QtWidgets/QWidget>

#include "ui_rigid_body_simulator.h"

class RigidBodySimulator : public QWidget
{
   Q_OBJECT

public:

   RigidBodySimulator(QWidget *parent = Q_NULLPTR);

public slots:

   void processSimulationError(int errorCode);

private slots:

   void onSceneComboBoxCurrentIndexChanged(int index);

   void onStartPausePushButtonClicked();
   void onResetPushButtonClicked();

   void onNoGravityRadioButtonToggled(bool checked);
   void onGravityRadioButtonToggled(bool checked);
   void onInvertedGravityRadioButtonToggled(bool checked);

   void onTimeStepSpinBoxValueChanged(double timeStep);
   void onCoefficientOfRestitutionSpinBoxValueChanged(double coefficientOfRestitution);

   void onWireframeModeCheckBoxToggled(bool checked);
   void onRememberFramesCheckBoxToggled(bool checked);
   void onRememberFramesSpinBoxValueChanged(int frequency);
   void onAntiAliasingModeCheckBoxToggled(bool checked);
   void onAntiAliasingModeComboBoxCurrentIndexChanged(int index);

signals:

   void changeScene(int index);

   void startSimulation();
   void pauseSimulation();
   void resetSimulation();

   void changeGravity(int state);

   void changeTimeStep(double timeStep);
   void changeCoefficientOfRestitution(double coefficientOfRestitution);

   void enableWireframeMode(bool enable);
   void enableRememberFrames(bool enable);
   void changeRememberFramesFrequency(int frequency);
   void enableAntiAliasing(bool enable);
   void changeAntiAliasingMode(int index);

private:

   Ui::RigidBodySimulatorClass ui;

   bool                        mSimulationIsRunning;

   QPalette                    mRunningPalette;
   QPalette                    mPausedPalette;
   QPalette                    mErrorPalette;
};

#endif
