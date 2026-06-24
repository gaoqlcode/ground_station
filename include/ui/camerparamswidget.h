/**
 * @file camerparamswidget.h
 * @brief 相机参数设置控件头文件
 *
 * 提供相机参数配置界面，包括分辨率、帧率、曝光时间和增益设置。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_CAMERPARAMSWIDGET_H
#define GROUND_STATION_CAMERPARAMSWIDGET_H

#include <QWidget>

class QComboBox;
class QSpinBox;
class QPushButton;
class QGroupBox;
class QGridLayout;

namespace ground_station {

/**
 * @class CameraParamsWidget
 * @brief 相机参数设置控件类
 *
 * 提供相机参数配置界面，支持分辨率选择、帧率设置、曝光时间和增益调整。
 */
class CameraParamsWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit CameraParamsWidget(QWidget *parent = nullptr);

private slots:
    /**
     * @brief 应用按钮点击处理
     */
    void onApplyClicked();

private:
    /**
     * @brief 设置用户界面
     */
    void setupUi();

    QGroupBox *m_paramsGroup;       // 参数设置分组框
    QComboBox *m_resolutionCombo;   // 分辨率下拉选择框
    QSpinBox *m_fpsSpinBox;         // 帧率输入框
    QSpinBox *m_exposureSpinBox;    // 曝光时间输入框
    QSpinBox *m_gainSpinBox;        // 增益输入框
    QPushButton *m_applyBtn;        // 应用参数按钮
};

} // namespace ground_station

#endif // GROUND_STATION_CAMERPARAMSWIDGET_H
