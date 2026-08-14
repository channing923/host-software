/*
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "measurement_workspace.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QStyle>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QTreeWidget>
#include <QVBoxLayout>

namespace
{
void RefreshStyle(QWidget* widget)
{
  widget->style()->unpolish(widget);
  widget->style()->polish(widget);
  widget->update();
}

QComboBox* AddCombo(QFormLayout* layout, const QString& label, const QStringList& values,
                    const QString& current = {})
{
  auto* combo = new QComboBox;
  combo->addItems(values);
  if (!current.isEmpty())
  {
    combo->setCurrentText(current);
  }
  layout->addRow(label, combo);
  return combo;
}

QDoubleSpinBox* AddDoubleSpin(QFormLayout* layout, const QString& label, double minimum,
                              double maximum, double value, const QString& suffix = {})
{
  auto* spin = new QDoubleSpinBox;
  spin->setRange(minimum, maximum);
  spin->setDecimals(3);
  spin->setValue(value);
  spin->setSuffix(suffix);
  layout->addRow(label, spin);
  return spin;
}

QSpinBox* AddSpin(QFormLayout* layout, const QString& label, int minimum, int maximum, int value,
                  const QString& suffix = {})
{
  auto* spin = new QSpinBox;
  spin->setRange(minimum, maximum);
  spin->setValue(value);
  spin->setSuffix(suffix);
  layout->addRow(label, spin);
  return spin;
}

QWidget* MakeScrollablePage(QWidget* content)
{
  auto* page = new QWidget;
  auto* page_layout = new QVBoxLayout(page);
  page_layout->setContentsMargins(0, 0, 0, 0);

  auto* scroll = new QScrollArea(page);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidget(content);
  page_layout->addWidget(scroll);
  return page;
}

QPushButton* AddApplyButton(QVBoxLayout* layout, const QString& page_name,
                            MeasurementConfigPanel* receiver)
{
  auto* button = new QPushButton(QObject::tr("应用"));
  button->setProperty("role", "primary");
  button->setProperty("requiresConnection", true);
  button->setEnabled(false);
  layout->addWidget(button, 0, Qt::AlignRight);
  QObject::connect(button, &QPushButton::clicked, receiver,
                   [receiver, page_name]() { emit receiver->applyRequested(page_name); });
  return button;
}

QToolButton* AddNavigationButton(QHBoxLayout* layout, const QString& text, bool selected = false)
{
  auto* button = new QToolButton;
  button->setText(text);
  button->setCheckable(true);
  button->setAutoExclusive(true);
  button->setChecked(selected);
  button->setProperty("role", "workspaceNavigation");
  layout->addWidget(button);
  return button;
}

QIcon ChannelIcon(const QColor& color)
{
  QPixmap pixmap(8, 8);
  pixmap.fill(color);
  return QIcon(pixmap);
}

QGroupBox* AddParameterGroup(QVBoxLayout* parent, const QString& title)
{
  auto* group = new QGroupBox(title);
  group->setObjectName("measurementParameterGroup");
  parent->addWidget(group);
  return group;
}
}  // namespace

MeasurementControlBar::MeasurementControlBar(QWidget* parent) : QFrame(parent)
{
  setObjectName("measurementControlBar");
  setFixedHeight(40);

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(8, 0, 8, 0);
  layout->setSpacing(0);

  AddNavigationButton(layout, tr("实时波形"), true);
  AddNavigationButton(layout, tr("触发采样"));
  AddNavigationButton(layout, tr("历史数据"));

  layout->addStretch();

  // The serial selectors are kept as the connection-state model used by the
  // communication layer. The visible controls live in the right connection page.
  _port_combo = new QComboBox(this);
  _refresh_button = new QToolButton(this);
  _baud_combo = new QComboBox(this);
  _baud_combo->setEditable(true);
  _baud_combo->addItems({"9600", "115200", "460800", "921600", "2000000"});
  _baud_combo->setCurrentText("2000000");
  _connection_button = new QPushButton(tr("连接"), this);
  _status_indicator = new QLabel(QString::fromUtf8("●"), this);
  _status_indicator->setObjectName("measurementStatusIndicator");
  _status_indicator->setProperty("status", "idle");
  _status_text = new QLabel(tr("未连接"), this);
  _device_summary = new QLabel(tr("未识别设备"), this);
  _port_combo->hide();
  _refresh_button->hide();
  _baud_combo->hide();
  _connection_button->hide();
  _status_indicator->hide();
  _status_text->hide();
  _device_summary->hide();

  _start_button = new QPushButton(tr("●  开始"), this);
  _start_button->setProperty("role", "primary");
  _pause_button = new QPushButton(tr("Ⅱ  暂停"), this);
  _pause_button->setCheckable(true);
  _stop_button = new QPushButton(tr("●  停止"), this);
  _stop_button->setProperty("role", "danger");
  _single_button = new QPushButton(tr("◎  单次"), this);
  _single_button->setProperty("role", "secondaryAction");
  _clear_button = new QPushButton(tr("清屏"), this);
  _clear_button->setProperty("role", "quietAction");

  layout->addWidget(_start_button);
  layout->addSpacing(6);
  layout->addWidget(_pause_button);
  layout->addSpacing(6);
  layout->addWidget(_single_button);
  layout->addSpacing(6);
  layout->addWidget(_stop_button);
  layout->addSpacing(6);
  layout->addWidget(_clear_button);

  connect(_refresh_button, &QToolButton::clicked, this,
          &MeasurementControlBar::refreshPortsRequested);
  connect(_connection_button, &QPushButton::clicked, this,
          [this]()
          {
            if (_connection_state == ConnectionState::Disconnected ||
                _connection_state == ConnectionState::Error)
            {
              emit connectionRequested(selectedPort(), selectedBaudRate());
            }
            else
            {
              emit disconnectionRequested();
            }
          });
  connect(_start_button, &QPushButton::clicked, this, &MeasurementControlBar::startRequested);
  connect(_pause_button, &QPushButton::toggled, this,
          &MeasurementControlBar::pauseDisplayRequested);
  connect(_stop_button, &QPushButton::clicked, this, &MeasurementControlBar::stopRequested);
  connect(_single_button, &QPushButton::clicked, this, &MeasurementControlBar::singleRequested);
  connect(_clear_button, &QPushButton::clicked, this, &MeasurementControlBar::clearRequested);

  setAvailablePorts({});
  refreshButtonStates();
}

QString MeasurementControlBar::selectedPort() const
{
  return _port_combo->currentData().toString().isEmpty() ? _port_combo->currentText()
                                                         : _port_combo->currentData().toString();
}

int MeasurementControlBar::selectedBaudRate() const
{
  return _baud_combo->currentText().remove(',').toInt();
}

void MeasurementControlBar::setAvailablePorts(const QStringList& ports)
{
  const QString previous = selectedPort();
  _port_combo->clear();
  if (ports.isEmpty())
  {
    _port_combo->addItem(tr("未检测到串口"));
    _port_combo->setEnabled(false);
  }
  else
  {
    _port_combo->addItems(ports);
    _port_combo->setEnabled(_connection_state == ConnectionState::Disconnected ||
                            _connection_state == ConnectionState::Error);
    const int previous_index = _port_combo->findText(previous);
    if (previous_index >= 0)
    {
      _port_combo->setCurrentIndex(previous_index);
    }
  }
  refreshButtonStates();
}

void MeasurementControlBar::setConnectionState(ConnectionState state, const QString& detail)
{
  _connection_state = state;

  QString status;
  QString text;
  switch (state)
  {
    case ConnectionState::Disconnected:
      status = "idle";
      text = tr("未连接");
      _connection_button->setText(tr("连接"));
      break;
    case ConnectionState::Connecting:
      status = "warning";
      text = tr("连接中");
      _connection_button->setText(tr("取消"));
      break;
    case ConnectionState::Connected:
      status = "good";
      text = tr("已连接");
      _connection_button->setText(tr("断开"));
      break;
    case ConnectionState::Reconnecting:
      status = "warning";
      text = tr("重连中");
      _connection_button->setText(tr("断开"));
      break;
    case ConnectionState::Error:
      status = "error";
      text = tr("连接错误");
      _connection_button->setText(tr("重试"));
      break;
  }

  if (!detail.isEmpty())
  {
    text += QString(" · %1").arg(detail);
  }
  _status_indicator->setProperty("status", status);
  _status_text->setText(text);
  RefreshStyle(_status_indicator);
  refreshButtonStates();
}

void MeasurementControlBar::setAcquisitionState(AcquisitionState state)
{
  _acquisition_state = state;
  const bool paused = state == AcquisitionState::DisplayPaused;
  if (_pause_button->isChecked() != paused)
  {
    _pause_button->blockSignals(true);
    _pause_button->setChecked(paused);
    _pause_button->blockSignals(false);
  }
  _pause_button->setText(paused ? tr("▶  继续显示") : tr("Ⅱ  暂停显示"));
  refreshButtonStates();
}

void MeasurementControlBar::setDeviceSummary(const QString& summary)
{
  _device_summary->setText(summary.isEmpty() ? tr("未识别设备") : summary);
  _device_summary->setToolTip(_device_summary->text());
}

void MeasurementControlBar::refreshButtonStates()
{
  const bool disconnected = _connection_state == ConnectionState::Disconnected ||
                            _connection_state == ConnectionState::Error;
  const bool connected = _connection_state == ConnectionState::Connected;
  const bool acquiring = _acquisition_state == AcquisitionState::Running ||
                         _acquisition_state == AcquisitionState::DisplayPaused ||
                         _acquisition_state == AcquisitionState::SingleShot;

  _port_combo->setEnabled(disconnected && _port_combo->count() > 0 &&
                          _port_combo->currentText() != tr("未检测到串口"));
  _baud_combo->setEnabled(disconnected);
  _refresh_button->setEnabled(disconnected);
  _connection_button->setEnabled(!disconnected || _port_combo->isEnabled());
  _start_button->setEnabled(connected && !acquiring);
  _single_button->setEnabled(connected && !acquiring);
  _pause_button->setEnabled(connected && acquiring);
  _stop_button->setEnabled(connected && acquiring);
  _clear_button->setEnabled(true);
}

MeasurementDevicePanel::MeasurementDevicePanel(QWidget* parent) : QFrame(parent)
{
  setObjectName("measurementDevicePanel");
  auto* outer = new QVBoxLayout(this);
  outer->setContentsMargins(0, 0, 0, 0);
  outer->setSpacing(0);

  auto* header_widget = new QWidget(this);
  header_widget->setObjectName("measurementPanelHeader");
  auto* header = new QHBoxLayout(header_widget);
  header->setContentsMargins(10, 0, 8, 0);
  auto* title = new QLabel(tr("设备与通道"), this);
  title->setObjectName("measurementPanelTitle");
  header->addWidget(title);
  header->addStretch();

  auto* add_button = new QToolButton(this);
  add_button->setText(QString::fromUtf8("＋"));
  add_button->setToolTip(tr("添加设备或通道"));
  header->addWidget(add_button);

  _status_indicator = new QLabel(QString::fromUtf8("●"), this);
  _status_indicator->setObjectName("measurementStatusIndicator");
  _status_indicator->setProperty("status", "idle");
  header->addWidget(_status_indicator);
  outer->addWidget(header_widget);

  auto* device_summary = new QWidget(this);
  device_summary->setObjectName("measurementDeviceSummaryBlock");
  auto* device_summary_layout = new QVBoxLayout(device_summary);
  device_summary_layout->setContentsMargins(10, 7, 10, 7);
  device_summary_layout->setSpacing(2);
  _device_name = new QLabel(tr("设备 1 · 多通道测量仪"), this);
  _device_name->setObjectName("measurementDeviceName");
  _device_detail = new QLabel(tr("等待连接 · 型号与序列号将在握手后读取"), this);
  _device_detail->setWordWrap(true);
  _device_detail->setObjectName("measurementSecondaryText");
  device_summary_layout->addWidget(_device_name);
  device_summary_layout->addWidget(_device_detail);
  outer->addWidget(device_summary);

  auto* tree = new QTreeWidget(this);
  tree->setObjectName("measurementChannelTree");
  tree->setColumnCount(2);
  tree->setHeaderHidden(true);
  tree->setIndentation(13);
  tree->setRootIsDecorated(true);
  tree->setAnimated(true);
  tree->setSelectionMode(QAbstractItemView::SingleSelection);
  tree->header()->setSectionResizeMode(0, QHeaderView::Stretch);
  tree->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

  auto* electrical = new QTreeWidgetItem(tree, {tr("电参数通道"), QString::fromUtf8("◉")});
  electrical->setExpanded(true);
  auto* hv1 = new QTreeWidgetItem(electrical, {tr("HV1  高压输入"), QString::fromUtf8("◉")});
  hv1->setIcon(0, ChannelIcon("#14cdd4"));
  auto* out1 = new QTreeWidgetItem(electrical, {tr("OUT1  低压输出"), QString::fromUtf8("◉")});
  out1->setIcon(0, ChannelIcon("#2d7df6"));

  auto* temperature = new QTreeWidgetItem(tree, {tr("温度通道"), QString::fromUtf8("◉")});
  temperature->setExpanded(true);
  const QList<QColor> channel_colors = {"#ff9f1c", "#e7d21b", "#49c85a", "#8f62e8"};
  for (int index = 0; index < channel_colors.size(); ++index)
  {
    auto* item = new QTreeWidgetItem(
        temperature, {QString("T%1").arg(index + 1), QString::fromUtf8("◉")});
    item->setIcon(0, ChannelIcon(channel_colors[index]));
  }

  auto* virtual_channels =
      new QTreeWidgetItem(tree, {tr("虚拟通道"), QString::fromUtf8("＋")});
  virtual_channels->setExpanded(true);
  auto* delta_t = new QTreeWidgetItem(virtual_channels, {tr("ΔT  T1 - T2"), QString::fromUtf8("◉")});
  delta_t->setIcon(0, ChannelIcon("#9da6b2"));
  outer->addWidget(tree, 1);

  auto* action_widget = new QWidget(this);
  action_widget->setObjectName("measurementPanelFooter");
  auto* actions = new QHBoxLayout(action_widget);
  actions->setContentsMargins(8, 5, 8, 5);
  actions->setSpacing(5);
  auto* virtual_button = new QPushButton(tr("＋ 虚拟通道"), this);
  auto* group_button = new QPushButton(tr("＋ 波形组"), this);
  virtual_button->setToolTip(tr("创建由物理通道或虚拟通道计算得到的通道"));
  group_button->setToolTip(tr("创建具有独立曲线集合和纵轴配置的波形组"));
  actions->addWidget(virtual_button);
  actions->addWidget(group_button);
  outer->addWidget(action_widget);

  connect(add_button, &QToolButton::clicked, this,
          &MeasurementDevicePanel::addVirtualChannelRequested);
  connect(virtual_button, &QPushButton::clicked, this,
          &MeasurementDevicePanel::addVirtualChannelRequested);
  connect(group_button, &QPushButton::clicked, this,
          &MeasurementDevicePanel::addWaveformGroupRequested);
}

void MeasurementDevicePanel::setDeviceSummary(const QString& name, const QString& detail,
                                              bool connected)
{
  _device_name->setText(name.isEmpty() ? tr("未连接设备") : name);
  _device_detail->setText(detail.isEmpty() ? tr("等待设备信息") : detail);
  _status_indicator->setProperty("status", connected ? "good" : "idle");
  RefreshStyle(_status_indicator);
}

MeasurementOverviewBar::MeasurementOverviewBar(QWidget* parent) : QFrame(parent)
{
  setObjectName("measurementOverviewBar");
  setFixedHeight(66);

  auto* layout = new QHBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 6);
  layout->setSpacing(6);

  auto add_metric = [this, layout](const QString& title, const QString& channel,
                                  const QString& unit, QLabel** value_label)
  {
    auto* card = new QFrame(this);
    card->setObjectName("measurementMetricCard");
    auto* card_layout = new QVBoxLayout(card);
    card_layout->setContentsMargins(9, 5, 9, 5);
    card_layout->setSpacing(0);

    auto* heading = new QLabel(QString("%1  %2").arg(title, channel), card);
    heading->setObjectName("measurementMetricTitle");
    *value_label = new QLabel(QString("-- <span style='font-size:9pt'>%1</span>").arg(unit), card);
    (*value_label)->setObjectName("measurementMetricValue");
    (*value_label)->setTextFormat(Qt::RichText);
    card_layout->addWidget(heading);
    card_layout->addWidget(*value_label);
    layout->addWidget(card, 1);
  };

  add_metric(tr("峰值电压"), "HV1", "V", &_peak_voltage);
  add_metric(tr("有效值"), "OUT1", "V", &_rms_voltage);
  add_metric(tr("频率"), "HV1", "kHz", &_frequency);
  add_metric(tr("最高温度"), "T1/T2/T3/T4", "°C", &_maximum_temperature);

  auto* tools = new QFrame(this);
  tools->setObjectName("measurementMetricTools");
  auto* tools_layout = new QHBoxLayout(tools);
  tools_layout->setContentsMargins(5, 0, 5, 0);
  tools_layout->setSpacing(3);
  for (const QString& glyph : {QString::fromUtf8("⌕"), QString::fromUtf8("⌖"),
                               QString::fromUtf8("↔"), QString::fromUtf8("▧")})
  {
    auto* button = new QToolButton(tools);
    button->setText(glyph);
    button->setFixedSize(25, 25);
    tools_layout->addWidget(button);
  }
  layout->addWidget(tools);
}

void MeasurementOverviewBar::setMetricValues(const QString& peak_voltage,
                                             const QString& rms_voltage,
                                             const QString& frequency,
                                             const QString& maximum_temperature)
{
  _peak_voltage->setText(peak_voltage);
  _rms_voltage->setText(rms_voltage);
  _frequency->setText(frequency);
  _maximum_temperature->setText(maximum_temperature);
}

MeasurementConfigPanel::MeasurementConfigPanel(QWidget* parent) : QFrame(parent)
{
  setObjectName("measurementConfigPanel");
  setMinimumWidth(252);
  setMaximumWidth(380);

  auto* layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  layout->setSpacing(0);

  _collapse_button = new QToolButton(this);
  _collapse_button->setText(QString::fromUtf8("›"));
  _collapse_button->setToolTip(tr("折叠配置栏"));
  _context_label = new QLabel(tr("当前对象：未选择"), this);
  _context_label->setObjectName("measurementContextLabel");
  _context_label->hide();

  _tabs = new QTabWidget(this);
  _tabs->setDocumentMode(true);
  _tabs->setObjectName("measurementConfigTabs");
  _tabs->addTab(createConnectionPage(), tr("连接"));
  _tabs->addTab(createAcquisitionPage(), tr("采集"));
  _tabs->addTab(createTriggerPage(), tr("触发"));
  _tabs->addTab(createParameterPage(), tr("参数"));
  _tabs->setCornerWidget(_collapse_button, Qt::TopRightCorner);
  layout->addWidget(_tabs, 1);

  connect(_collapse_button, &QToolButton::clicked, this, [this]() { setCollapsed(!_collapsed); });
  setConnected(false);
}

void MeasurementConfigPanel::setConnected(bool connected)
{
  const auto widgets = findChildren<QWidget*>();
  for (auto* widget : widgets)
  {
    if (widget->property("requiresConnection").toBool())
    {
      widget->setEnabled(connected);
    }
  }
}

void MeasurementConfigPanel::setContextText(const QString& text)
{
  _context_label->setText(text.isEmpty() ? tr("当前对象：未选择") : text);
}

void MeasurementConfigPanel::setCollapsed(bool collapsed)
{
  if (_collapsed == collapsed)
  {
    return;
  }
  _collapsed = collapsed;
  _tabs->setVisible(!collapsed);
  _context_label->setVisible(!collapsed);
  _collapse_button->setText(collapsed ? QString::fromUtf8("‹") : QString::fromUtf8("›"));
  _collapse_button->setToolTip(collapsed ? tr("展开配置栏") : tr("折叠配置栏"));
  setMinimumWidth(collapsed ? 34 : 252);
  setMaximumWidth(collapsed ? 34 : 380);
  emit collapsedChanged(collapsed);
}

bool MeasurementConfigPanel::isCollapsed() const
{
  return _collapsed;
}

QWidget* MeasurementConfigPanel::createConnectionPage()
{
  auto* content = new QWidget;
  auto* layout = new QVBoxLayout(content);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  auto* connection_group = AddParameterGroup(layout, tr("串口连接"));
  auto* form = new QFormLayout(connection_group);
  AddCombo(form, tr("串口"), {"COM5", "COM6", "ttyUSB0"}, "COM5");
  AddCombo(form, tr("波特率"), {"115200", "921600", "2000000"}, "2000000");
  AddCombo(form, tr("数据位"), {"8"});
  AddCombo(form, tr("校验"), {tr("无"), tr("奇校验"), tr("偶校验")});

  auto* state = new QLabel(tr("●  设备未连接"), content);
  state->setObjectName("measurementConnectionState");
  layout->addWidget(state);

  auto* button = new QPushButton(tr("连接设备"), content);
  button->setProperty("role", "primary");
  connect(button, &QPushButton::clicked, this,
          [this]() { emit applyRequested("connection"); });
  layout->addWidget(button);
  layout->addStretch();
  return MakeScrollablePage(content);
}

QWidget* MeasurementConfigPanel::createAcquisitionPage()
{
  auto* content = new QWidget;
  content->setProperty("requiresConnection", true);
  auto* layout = new QVBoxLayout(content);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  auto* source_group = AddParameterGroup(layout, tr("采样设置"));
  auto* form = new QFormLayout(source_group);
  AddCombo(form, tr("采样模式"), {tr("连续流"), tr("缓冲"), tr("单次")}, tr("连续流"));
  AddCombo(form, tr("采样率"), {"10 kS/s", "50 kS/s", "100 kS/s"}, "100 kS/s");
  AddCombo(form, tr("采样深度"), {tr("10 k点"), tr("100 k点"), tr("1 M点")}, tr("100 k点"));

  auto* upload_group = AddParameterGroup(layout, tr("数据上传"));
  auto* upload_form = new QFormLayout(upload_group);
  AddSpin(upload_form, tr("上传周期"), 10, 5000, 100, " ms");
  AddCombo(upload_form, tr("传输格式"), {tr("二进制帧"), "MessagePack"});
  layout->addStretch();
  AddApplyButton(layout, "acquisition", this);
  return MakeScrollablePage(content);
}

QWidget* MeasurementConfigPanel::createTriggerPage()
{
  auto* content = new QWidget;
  content->setProperty("requiresConnection", true);
  auto* layout = new QVBoxLayout(content);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  auto* condition_group = AddParameterGroup(layout, tr("触发条件"));
  auto* form = new QFormLayout(condition_group);
  AddCombo(form, tr("触发通道"), {"HV1", "OUT1", "T1", "T2"}, "HV1");
  AddCombo(form, tr("触发类型"), {tr("边沿"), tr("窗口"), tr("过压"), tr("过温")});
  AddCombo(form, tr("边沿"), {tr("上升沿"), tr("下降沿")});
  AddDoubleSpin(form, tr("阈值"), -1000000.0, 1000000.0, 350.0, " V");

  auto* behavior_group = AddParameterGroup(layout, tr("触发行为"));
  auto* behavior_form = new QFormLayout(behavior_group);
  AddSpin(behavior_form, tr("预触发"), 0, 100, 30, " %");
  AddCombo(behavior_form, tr("触发方式"), {tr("单次"), tr("正常"), tr("自动")}, tr("正常"));
  layout->addStretch();
  AddApplyButton(layout, "trigger", this);
  return MakeScrollablePage(content);
}

QWidget* MeasurementConfigPanel::createParameterPage()
{
  auto* content = new QWidget;
  auto* layout = new QVBoxLayout(content);
  layout->setContentsMargins(8, 8, 8, 8);
  layout->setSpacing(6);

  auto* channel_group = AddParameterGroup(layout, tr("通道设置"));
  auto* channel_form = new QFormLayout(channel_group);
  AddCombo(channel_form, tr("通道"), {"HV1", "OUT1", "T1", "T2", "T3", "T4"}, "HV1");
  AddCombo(channel_form, tr("量程"), {"±10 V", "±100 V", "±1000 V"}, "±1000 V");
  AddCombo(channel_form, tr("探头比"), {"1:1", "10:1", "100:1", "1000:1"}, "1000:1");
  AddDoubleSpin(channel_form, tr("偏移"), -100000.0, 100000.0, 0.0);

  auto* display_group = AddParameterGroup(layout, tr("显示设置"));
  auto* display_form = new QFormLayout(display_group);
  AddCombo(display_form, tr("模式"), {tr("滚动"), tr("扫屏"), tr("固定窗口")}, tr("滚动"));
  AddSpin(display_form, tr("刷新率"), 1, 60, 25, " Hz");
  AddCombo(display_form, tr("时间轴"), {tr("同步"), tr("独立")});

  auto* processing_group = AddParameterGroup(layout, tr("信号处理"));
  auto* processing_form = new QFormLayout(processing_group);
  AddCombo(processing_form, tr("滤波"), {tr("关闭"), tr("低通"), tr("高通"), tr("50 Hz 陷波")});
  AddCombo(processing_form, "FFT", {tr("关闭"), "1024", "2048", "4096"});

  auto* alarm_group = AddParameterGroup(layout, tr("报警阈值"));
  auto* alarm_form = new QFormLayout(alarm_group);
  AddDoubleSpin(alarm_form, tr("高限"), -1000000.0, 1000000.0, 80.0);
  AddDoubleSpin(alarm_form, tr("低限"), -1000000.0, 1000000.0, -20.0);
  layout->addStretch();
  AddApplyButton(layout, "parameter", this);
  return MakeScrollablePage(content);
}

MeasurementFaultPanel::MeasurementFaultPanel(QWidget* parent) : QFrame(parent)
{
  setObjectName("measurementFaultPanel");
  setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

  auto* outer = new QVBoxLayout(this);
  outer->setContentsMargins(7, 0, 7, 4);
  outer->setSpacing(2);

  auto* summary = new QHBoxLayout;
  summary->setSpacing(8);
  _expand_button = new QToolButton(this);
  _expand_button->setText(QString::fromUtf8("⌃"));
  _expand_button->setToolTip(tr("展开故障列表"));
  summary->addWidget(_expand_button);

  _connection_indicator = new QLabel(QString::fromUtf8("●"), this);
  _connection_indicator->setObjectName("measurementStatusIndicator");
  _connection_indicator->setProperty("status", "idle");
  _connection_text = new QLabel(tr("设备未连接"), this);
  summary->addWidget(_connection_indicator);
  summary->addWidget(_connection_text);

  auto add_separator = [this, summary]()
  {
    auto* separator = new QFrame(this);
    separator->setFrameShape(QFrame::VLine);
    separator->setObjectName("measurementDivider");
    summary->addWidget(separator);
  };
  auto add_metric = [this, summary](const QString& title, const QString& initial)
  {
    auto* label = new QLabel(QString("%1  %2").arg(title, initial), this);
    label->setObjectName("measurementStatusMetric");
    summary->addWidget(label);
    return label;
  };

  add_separator();
  _throughput = add_metric(tr("有效带宽"), "-- kB/s");
  add_separator();
  _sample_rate = add_metric(tr("采样"), "-- kS/s");
  add_separator();
  _frame_count = add_metric(tr("帧"), "0");
  add_separator();
  _crc_errors = add_metric(tr("CRC错误"), "0");
  add_separator();
  _runtime = add_metric(tr("运行"), "00:00:00");
  summary->addStretch();

  _alarm_summary = new QLabel(tr("无活动告警"), this);
  _alarm_summary->setObjectName("measurementAlarmSummary");
  summary->addWidget(_alarm_summary);
  outer->addLayout(summary);

  _fault_table = new QTableWidget(0, 7, this);
  _fault_table->setHorizontalHeaderLabels({tr("发生时间"), tr("设备/通道"), tr("级别"),
                                           tr("故障类型"), tr("测量值/阈值"), tr("状态"),
                                           tr("说明")});
  _fault_table->setSelectionBehavior(QAbstractItemView::SelectRows);
  _fault_table->setSelectionMode(QAbstractItemView::SingleSelection);
  _fault_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _fault_table->verticalHeader()->setVisible(false);
  _fault_table->horizontalHeader()->setStretchLastSection(true);
  _fault_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  outer->addWidget(_fault_table, 1);

  connect(_expand_button, &QToolButton::clicked, this, [this]() { setExpanded(!_expanded); });
  connect(_fault_table, &QTableWidget::cellDoubleClicked, this,
          [this](int row, int)
          {
            auto* time_item = _fault_table->item(row, 0);
            auto* channel_item = _fault_table->item(row, 1);
            if (time_item && channel_item)
            {
              emit faultActivated(time_item->data(Qt::UserRole).toDouble(), channel_item->text());
            }
          });

  setExpanded(false);
}

void MeasurementFaultPanel::setExpanded(bool expanded)
{
  _expanded = expanded;
  _fault_table->setVisible(expanded);
  _expand_button->setText(expanded ? QString::fromUtf8("⌄") : QString::fromUtf8("⌃"));
  _expand_button->setToolTip(expanded ? tr("折叠故障列表") : tr("展开故障列表"));
  setFixedHeight(expanded ? 210 : 30);
  emit expandedChanged(expanded);
}

bool MeasurementFaultPanel::isExpanded() const
{
  return _expanded;
}

void MeasurementFaultPanel::setConnectionSummary(const QString& text, const QString& status)
{
  _connection_text->setText(text);
  _connection_indicator->setProperty("status", status);
  RefreshStyle(_connection_indicator);
}

void MeasurementFaultPanel::setRuntimeStatistics(const QString& throughput,
                                                 const QString& sample_rate, quint64 frame_count,
                                                 quint64 crc_errors, const QString& runtime)
{
  _throughput->setText(tr("有效带宽  %1").arg(throughput));
  _sample_rate->setText(tr("采样  %1").arg(sample_rate));
  _frame_count->setText(tr("帧  %1").arg(frame_count));
  _crc_errors->setText(tr("CRC错误  %1").arg(crc_errors));
  _runtime->setText(tr("运行  %1").arg(runtime));
}
