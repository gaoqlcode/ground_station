/**
 * @file camerparamswidget.cpp
 * @brief 相机参数设置控件实现
 *
 * 本文件实现了CameraParamsWidget类，提供相机参数配置界面。
 * 主要功能包括：
 * - 分辨率选择
 * - 帧率设置
 * - 曝光时间调整
 * - 增益控制
 * - 参数应用按钮
 *
 * @author GroundStation Team
 * @date 2024
 */

#include "camerparamswidget.h"

#include <QGridLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QMessageBox>

namespace ground_station {

/**
 * @brief 构造函数
 *
 * 初始化相机参数设置控件，创建界面布局和控件。
 *
 * @param parent 父控件指针
 */
CameraParamsWidget::CameraParamsWidget(QWidget *parent)
    : QWidget(parent)
    , m_paramsGroup(nullptr)          // 参数设置分组框
    , m_resolutionCombo(nullptr)      // 分辨率下拉选择框
    , m_fpsSpinBox(nullptr)           // 帧率输入框
    , m_exposureSpinBox(nullptr)      // 曝光时间输入框
    , m_gainSpinBox(nullptr)          // 增益输入框
    , m_applyBtn(nullptr)             // 应用参数按钮
{
    setupUi();                        // 创建界面控件
}

/**
 * @brief 创建界面布局
 *
 * 创建相机参数设置界面，包含分辨率、帧率、曝光和增益四个参数的输入控件，
 * 以及应用按钮。
 */
void CameraParamsWidget::setupUi()
{
    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(8);

    m_paramsGroup = new QGroupBox(tr("相机参数设置栏"), this);
    QGridLayout *groupLayout = new QGridLayout(m_paramsGroup);
    groupLayout->setContentsMargins(12, 16, 12, 12);
    groupLayout->setSpacing(10);
    groupLayout->setColumnStretch(1, 1);
    groupLayout->setColumnStretch(3, 1);

    groupLayout->addWidget(new QLabel(tr("分辨率:"), this), 0, 0);
    m_resolutionCombo = new QComboBox(this);
    m_resolutionCombo->addItems({
        tr("1920×1080"),
        tr("1280×720"),
        tr("640×480")
    });
    groupLayout->addWidget(m_resolutionCombo, 0, 1);

    groupLayout->addWidget(new QLabel(tr("帧率 (fps):"), this), 0, 2);
    m_fpsSpinBox = new QSpinBox(this);
    m_fpsSpinBox->setRange(1, 120);
    m_fpsSpinBox->setValue(30);
    groupLayout->addWidget(m_fpsSpinBox, 0, 3);

    groupLayout->addWidget(new QLabel(tr("曝光:"), this), 1, 0);
    m_exposureSpinBox = new QSpinBox(this);
    m_exposureSpinBox->setRange(1, 10000);
    m_exposureSpinBox->setValue(100);
    m_exposureSpinBox->setSuffix(tr(" μs"));
    groupLayout->addWidget(m_exposureSpinBox, 1, 1);

    groupLayout->addWidget(new QLabel(tr("增益:"), this), 1, 2);
    m_gainSpinBox = new QSpinBox(this);
    m_gainSpinBox->setRange(0, 100);
    m_gainSpinBox->setValue(10);
    m_gainSpinBox->setSuffix(tr(" dB"));
    groupLayout->addWidget(m_gainSpinBox, 1, 3);

    m_applyBtn = new QPushButton(tr("应用参数"), this);
    m_applyBtn->setMinimumWidth(120);
    groupLayout->addWidget(m_applyBtn, 1, 4);

    mainLayout->addWidget(m_paramsGroup, 0, 0);

    connect(m_applyBtn, &QPushButton::clicked, this, &CameraParamsWidget::onApplyClicked);
}

void CameraParamsWidget::onApplyClicked()
{
    QMessageBox::information(this, tr("相机参数"),
        tr("参数已提交：\n分辨率: %1\n帧率: %2 fps\n曝光: %3\n增益: %4")
            .arg(m_resolutionCombo->currentText())
            .arg(m_fpsSpinBox->value())
            .arg(m_exposureSpinBox->value())
            .arg(m_gainSpinBox->value()));
}

} // namespace ground_station
