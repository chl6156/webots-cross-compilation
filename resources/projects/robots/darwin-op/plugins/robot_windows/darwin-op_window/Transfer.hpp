/*
 * File:         Transfer.hpp
 * Date:         January 2013
 * Description:  Widget for communicating with the robot DARwIn-OP
 * Author:       david.mansolino@epfl.ch
 * Modifications:
 */

#ifndef TRANSFER_HPP
#define TRANSFER_HPP

#include <QtGui/QtGui>
#include <QtCore/QtCore>
#include <QtWidgets/QtWidgets>

#include <pthread.h>

class SSH;
struct Parameters;

class Transfer : public QScrollArea
{

  Q_OBJECT

public:
  Transfer(QWidget *parent = 0);
  virtual ~Transfer();
  enum Status { DISCONNECTED,
                START_REMOTE_CONTROL,
                RUN_REMOTE_CONTROL,
                STOP_REMOTE_CONTROL,
                UNINSTALL,
                START_REMOTE_COMPILATION,
                RUN_REMOTE_COMPILATION,
                STOP_REMOTE_COMPILATION };
  void saveSettings();
  void robotInstableSlot();
  void enableButtons();
  void disableButtons();
public slots:
  void installControllerWarningSlot();
  void restoreSettings();
  void uninstall();
  void sendController();
  void remoteControl();
  void timerCallback();
  void SSHSessionComplete();
  void SSHSessionDone();
  void print(const QString &,bool err);
  void status(const QString &);

signals:
  void setStabilityResponseSignal(int stability);
  void isRobotStableSignal();
  void stopRemoteSignal();
  void stopControllerSignal();
  void testSignal();

private:
  void showProgressBox(const QString &title,const QString &message);
  void finishStartRemoteCompilation();
  void finishStartRemoteControl();
  void finish();
  QFutureWatcher<int> mFutureWatcher;
  QFuture<int> mFuture;
  SSH         *mSSH;
  Status       mStatus;

  //***  SSH  ***//
  bool         mConnectionState;

  QWidget     *mContainerWidget;
  QGridLayout *mContainerGridLayout;

  //***  SETTINGS  ***//
  QGridLayout *mSettingsGridLayout;
  QGroupBox   *mSettingsGroupBox;

  // IP adresse
  QLineEdit   *mIPAddressLineEdit;
  QLabel      *mIPLabel;

  // Username
  QLineEdit   *mUsernameLineEdit;
  QLabel      *mUsernameLabel;

  // Password
  QLineEdit   *mPasswordLineEdit;
  QLabel      *mPasswordLabel;

  // Restore
  QPushButton *mDefaultSettingsButton;
  QSettings   *mSettings;

  //***  Upload controller  ***//
  QGridLayout *mActionGridLayout;
  QGroupBox   *mActionGroupBox;

  // Controller
  QIcon       *mSendControllerIcon;
  QIcon       *mStopControllerIcon;
  QPushButton *mSendControllerButton;
  QCheckBox   *mMakeDefaultControllerCheckBox;

  // remote control
  QIcon       *mRemoteControlIcon;
  QPushButton *mRemoteControlButton;
  bool         mRemoteEnable;

  // Remote control wait during starting
  QProgressDialog *mRemoteProgressDialog;
  QProgressBar    *mRemoteProgressBar;
  QTimer          *mTimer;

  // Wrapper
  QPushButton *mUninstallButton;

  //***  OUTPUT  ***//
  QGridLayout *mOutputGridLayout;
  QGroupBox   *mOutputGroupBox;

  // Status label and progress bar
  QLabel      *mStatusLabel;
  QProgressBar *mProgressBar;

  // Console Show
  QTextEdit   *mConsoleShowTextEdit;

  void         loadSettings();
};

#endif
