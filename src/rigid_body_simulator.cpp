#include <QComboBox>
#include <QDoubleSpinBox>

#include "rigid_body_simulator.h"

RigidBodySimulator::RigidBodySimulator(QWidget *parent)
   : QWidget(parent)
   , mSimulationIsRunning(false)
{
   ui.setupUi(this);

   // Green
   mRunningPalette.setColor(QPalette::Window, QColor(42, 42, 42));
   mRunningPalette.setColor(QPalette::WindowText, QColor(46, 204, 113));

   // Blue
   mPausedPalette.setColor(QPalette::Window, QColor(42, 42, 42));
   mPausedPalette.setColor(QPalette::WindowText, QColor(33, 150, 243));

   // Red
   mErrorPalette.setColor(QPalette::Window, QColor(42, 42, 42));
   mErrorPalette.setColor(QPalette::WindowText, QColor(255, 61, 0));

   ui.statusLabel->setFrameStyle(QFrame::StyledPanel);
   ui.statusLabel->setAutoFillBackground(true);
   ui.statusLabel->setPalette(mPausedPalette);
   ui.statusLabel->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
   ui.statusLabel->setText("Paused");

   ui.startPausePushButton->setIcon(QIcon(":RigidBodySimulator/icons/play.png"));
   ui.startPausePushButton->setIconSize(QSize(20, 20));
   ui.resetPushButton->setIcon(QIcon(":RigidBodySimulator/icons/reset.png"));
   ui.resetPushButton->setIconSize(QSize(20, 20));

   ui.sceneComboBox->addItem("Single");
   ui.sceneComboBox->addItem("Pair");
   ui.sceneComboBox->addItem("Momentum");
   ui.sceneComboBox->addItem("Torque");
   ui.sceneComboBox->addItem("Plus Sign");
   ui.sceneComboBox->addItem("Multiplication Sign");
   ui.sceneComboBox->addItem("Star");
   ui.sceneComboBox->addItem("Stack");
   ui.sceneComboBox->addItem("Stack Being Hit");
   ui.sceneComboBox->addItem("Hexagon");
   ui.sceneComboBox->addItem("Octagon");
   ui.sceneComboBox->addItem("Downward Slope");
   ui.sceneComboBox->addItem("Upward Slope");

   ui.antiAliasingModeComboBox->addItem("2x MSAA");
   ui.antiAliasingModeComboBox->addItem("4x MSAA");
   ui.antiAliasingModeComboBox->addItem("8x MSAA");

   setWindowFlags(Qt::Widget | Qt::MSWindowsFixedSizeDialogHint);

   // Simulation
   connect(ui.sceneComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &RigidBodySimulator::onSceneComboBoxCurrentIndexChanged);

   // Controls
   connect(ui.startPausePushButton, &QAbstractButton::clicked, this, &RigidBodySimulator::onStartPausePushButtonClicked);
   connect(ui.resetPushButton,      &QAbstractButton::clicked, this, &RigidBodySimulator::onResetPushButtonClicked);

   // Forces
   connect(ui.noGravityRadioButton,       &QAbstractButton::toggled, this, &RigidBodySimulator::onNoGravityRadioButtonToggled);
   connect(ui.gravityRadioButton,         &QAbstractButton::toggled, this, &RigidBodySimulator::onGravityRadioButtonToggled);
   connect(ui.invertedGravityRadioButton, &QAbstractButton::toggled, this, &RigidBodySimulator::onInvertedGravityRadioButtonToggled);

   // Constants
   connect(ui.timeStepSpinBox,                 qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RigidBodySimulator::onTimeStepSpinBoxValueChanged);
   connect(ui.coefficientOfRestitutionSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &RigidBodySimulator::onCoefficientOfRestitutionSpinBoxValueChanged);

   // Display
   connect(ui.wireFrameModeCheckBox,    &QAbstractButton::toggled,                       this, &RigidBodySimulator::onWireframeModeCheckBoxToggled);
   connect(ui.rememberFramesCheckBox,   &QAbstractButton::toggled,                       this, &RigidBodySimulator::onRememberFramesCheckBoxToggled);
   connect(ui.rememberFramesSpinBox,    qOverload<int>(&QSpinBox::valueChanged),         this, &RigidBodySimulator::onRememberFramesSpinBoxValueChanged);
   connect(ui.antiAliasingModeCheckBox, &QAbstractButton::toggled,                       this, &RigidBodySimulator::onAntiAliasingModeCheckBoxToggled);
   connect(ui.antiAliasingModeComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &RigidBodySimulator::onAntiAliasingModeComboBoxCurrentIndexChanged);
   connect(ui.recordGIFCheckBox,        &QAbstractButton::toggled,                       this, &RigidBodySimulator::onRecordGIFCheckBoxToggled);
}

void RigidBodySimulator::onSceneComboBoxCurrentIndexChanged(int index)
{
   //if (mSimulationIsRunning)
   //{
   //   ui.startPausePushButton->setIcon(QIcon(":RigidBodySimulator/icons/play.png"));
   //   ui.statusLabel->setPalette(mPausedPalette);
   //   ui.statusLabel->setText("Paused");
   //   mSimulationIsRunning = false;
   //}

   ui.startPausePushButton->setIcon(QIcon(":RigidBodySimulator/icons/play.png"));
   ui.statusLabel->setPalette(mPausedPalette);
   ui.statusLabel->setText("Paused");
   ui.recordGIFCheckBox->setEnabled(true);
   mSimulationIsRunning = false;

   switch (index)
   {
      case 0:  // Single
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(25);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 1:  // Pair
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(25);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 2:  // Momentum
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.015f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(50);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 3:  // Torque
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(100);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 4:  // Plus Sign
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(100);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(false);
         break;
      case 5:  // Multiplication Sign
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(100);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(false);
         break;
      case 6:  // Star
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.010f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(1);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(false);
         break;
      case 7:  // Stack
         ui.gravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(0.500f);
         ui.rememberFramesSpinBox->setValue(100);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 8:  // Stack Being Hit
         ui.gravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(1);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 9:  // Hexagon
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(25);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 10: // Octagon
         ui.noGravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(100);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 11: // Downward Slope
         ui.gravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(0.950f);
         ui.rememberFramesSpinBox->setValue(10);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
      case 12: // Upward Slope
         ui.gravityRadioButton->setChecked(true);
         ui.timeStepSpinBox->setValue(0.020f);
         ui.coefficientOfRestitutionSpinBox->setValue(1.000f);
         ui.rememberFramesSpinBox->setValue(10);
         ui.rememberFramesCheckBox->setChecked(false);
         ui.wireFrameModeCheckBox->setChecked(true);
         break;
   }

   emit changeScene(index);
}

void RigidBodySimulator::onStartPausePushButtonClicked()
{
   if (mSimulationIsRunning)
   {
      ui.startPausePushButton->setIcon(QIcon(":RigidBodySimulator/icons/play.png"));
      ui.statusLabel->setPalette(mPausedPalette);
      ui.statusLabel->setText("Paused");
      ui.recordGIFCheckBox->setEnabled(true);
      mSimulationIsRunning = false;

      emit pauseSimulation();
   }
   else
   {
      ui.startPausePushButton->setIcon(QIcon(":RigidBodySimulator/icons/pause.png"));
      ui.statusLabel->setPalette(mRunningPalette);
      ui.statusLabel->setText("Running");
      ui.recordGIFCheckBox->setEnabled(false);
      mSimulationIsRunning = true;

      emit startSimulation();
   }
}

void RigidBodySimulator::onResetPushButtonClicked()
{
   ui.startPausePushButton->setIcon(QIcon(":RigidBodySimulator/icons/play.png"));
   ui.statusLabel->setPalette(mPausedPalette);
   ui.statusLabel->setText("Paused");
   ui.recordGIFCheckBox->setEnabled(true);
   mSimulationIsRunning = false;

   emit resetSimulation();
}

void RigidBodySimulator::onNoGravityRadioButtonToggled(bool checked)
{
   if (checked)
   {
      emit changeGravity(0);
   }
}

void RigidBodySimulator::onGravityRadioButtonToggled(bool checked)
{
   if (checked)
   {
      emit changeGravity(1);
   }
}

void RigidBodySimulator::onInvertedGravityRadioButtonToggled(bool checked)
{
   if (checked)
   {
      emit changeGravity(2);
   }
}

void RigidBodySimulator::onTimeStepSpinBoxValueChanged(double timeStep)
{
   emit changeTimeStep(timeStep);
}

void RigidBodySimulator::onCoefficientOfRestitutionSpinBoxValueChanged(double coefficientOfRestitution)
{
   emit changeCoefficientOfRestitution(coefficientOfRestitution);
}

void RigidBodySimulator::onWireframeModeCheckBoxToggled(bool checked)
{
   emit enableWireframeMode(checked);
}

void RigidBodySimulator::onRememberFramesCheckBoxToggled(bool checked)
{
   emit enableRememberFrames(checked);
}

void RigidBodySimulator::onRememberFramesSpinBoxValueChanged(int frequency)
{
   emit changeRememberFramesFrequency(frequency);
}

void RigidBodySimulator::onAntiAliasingModeCheckBoxToggled(bool checked)
{
   emit enableAntiAliasing(checked);
}

void RigidBodySimulator::onAntiAliasingModeComboBoxCurrentIndexChanged(int index)
{
   emit changeAntiAliasingMode(index);
}

void RigidBodySimulator::onRecordGIFCheckBoxToggled(bool checked)
{
   emit enableRecordGIF(checked);
}

void RigidBodySimulator::processSimulationError(int errorCode)
{
   ui.startPausePushButton->setIcon(QIcon(":RigidBodySimulator/icons/play.png"));
   ui.statusLabel->setPalette(mErrorPalette);
   switch (errorCode)
   {
   case 1: ui.statusLabel->setText("Unresolvable Penetration");              break; // Unresolvable penetration error
   case 2: ui.statusLabel->setText("Unresolvable Body-Body\nCollision");     break; // Unresolvable body-body collision error
   case 3: ui.statusLabel->setText("Unresolvable Vertex-Vertex\nCollision"); break; // Unresolvable vertex-vertex collision error
   case 4: ui.statusLabel->setText("Unresolvable Vertex-Edge\nCollision");   break; // Unresolvable vertex-edge collision error
   }

   mSimulationIsRunning = false;
}
