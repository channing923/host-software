/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <QFrame>
#include <QStringList>

class QComboBox;
class QLabel;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QToolButton;
class QTreeWidget;
class QWidget;

class MeasurementControlBar : public QFrame
{
  Q_OBJECT

public:
  enum class ConnectionState
  {
    Disconnected,
    Connecting,
    Connected,
    Reconnecting,
    Error
  };

  enum class AcquisitionState
  {
    Idle,
    Running,
    DisplayPaused,
    SingleShot
  };

  explicit MeasurementControlBar(QWidget* parent = nullptr);

  QString selectedPort() const;
  int selectedBaudRate() const;

  void setAvailablePorts(const QStringList& ports);
  void setConnectionState(ConnectionState state, const QString& detail = {});
  void setAcquisitionState(AcquisitionState state);
  void setDeviceSummary(const QString& summary);

signals:
  void refreshPortsRequested();
  void connectionRequested(const QString& port, int baud_rate);
  void disconnectionRequested();
  void startRequested();
  void pauseDisplayRequested(bool paused);
  void stopRequested();
  void singleRequested();
  void clearRequested();

private:
  void refreshButtonStates();

  QComboBox* _port_combo = nullptr;
  QComboBox* _baud_combo = nullptr;
  QToolButton* _refresh_button = nullptr;
  QPushButton* _connection_button = nullptr;
  QPushButton* _start_button = nullptr;
  QPushButton* _pause_button = nullptr;
  QPushButton* _stop_button = nullptr;
  QPushButton* _single_button = nullptr;
  QPushButton* _clear_button = nullptr;
  QLabel* _status_indicator = nullptr;
  QLabel* _status_text = nullptr;
  QLabel* _device_summary = nullptr;

  ConnectionState _connection_state = ConnectionState::Disconnected;
  AcquisitionState _acquisition_state = AcquisitionState::Idle;
};

class MeasurementDevicePanel : public QFrame
{
  Q_OBJECT

public:
  explicit MeasurementDevicePanel(QWidget* parent = nullptr);

  void setDeviceSummary(const QString& name, const QString& detail, bool connected);

signals:
  void addVirtualChannelRequested();
  void addWaveformGroupRequested();

private:
  QLabel* _status_indicator = nullptr;
  QLabel* _device_name = nullptr;
  QLabel* _device_detail = nullptr;
};

class MeasurementOverviewBar : public QFrame
{
  Q_OBJECT

public:
  explicit MeasurementOverviewBar(QWidget* parent = nullptr);

  void setMetricValues(const QString& peak_voltage, const QString& rms_voltage,
                       const QString& frequency, const QString& maximum_temperature);

private:
  QLabel* _peak_voltage = nullptr;
  QLabel* _rms_voltage = nullptr;
  QLabel* _frequency = nullptr;
  QLabel* _maximum_temperature = nullptr;
};

class MeasurementConfigPanel : public QFrame
{
  Q_OBJECT

public:
  explicit MeasurementConfigPanel(QWidget* parent = nullptr);

  void setConnected(bool connected);
  void setContextText(const QString& text);
  void setCollapsed(bool collapsed);
  bool isCollapsed() const;

signals:
  void collapsedChanged(bool collapsed);
  void applyRequested(const QString& page_name);

private:
  QWidget* createConnectionPage();
  QWidget* createAcquisitionPage();
  QWidget* createTriggerPage();
  QWidget* createParameterPage();

  QTabWidget* _tabs = nullptr;
  QLabel* _context_label = nullptr;
  QToolButton* _collapse_button = nullptr;
  bool _collapsed = false;
};

class MeasurementFaultPanel : public QFrame
{
  Q_OBJECT

public:
  explicit MeasurementFaultPanel(QWidget* parent = nullptr);

  void setExpanded(bool expanded);
  bool isExpanded() const;
  void setConnectionSummary(const QString& text, const QString& status);
  void setRuntimeStatistics(const QString& throughput, const QString& sample_rate,
                            quint64 frame_count, quint64 crc_errors, const QString& runtime);

signals:
  void expandedChanged(bool expanded);
  void faultActivated(double timestamp, const QString& channel);

private:
  QToolButton* _expand_button = nullptr;
  QLabel* _connection_indicator = nullptr;
  QLabel* _connection_text = nullptr;
  QLabel* _throughput = nullptr;
  QLabel* _sample_rate = nullptr;
  QLabel* _frame_count = nullptr;
  QLabel* _crc_errors = nullptr;
  QLabel* _runtime = nullptr;
  QLabel* _alarm_summary = nullptr;
  QTableWidget* _fault_table = nullptr;
  bool _expanded = false;
};
