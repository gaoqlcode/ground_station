/**
 * @file cameralistwidget.h
 * @brief 相机列表控件头文件
 *
 * 提供相机列表的显示、管理和搜索功能，支持相机状态实时更新。
 *
 * @author GroundStation Team
 * @date 2024
 */

#ifndef GROUND_STATION_CAMERALISTWIDGET_H
#define GROUND_STATION_CAMERALISTWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QTimer>

namespace ground_station {

/**
 * @struct CameraInfo
 * @brief 相机信息结构
 */
struct CameraInfo {
    QString id;          ///< 相机ID
    QString name;        ///< 相机名称
    QString type;        ///< 相机类型
    QString status;      ///< 相机状态
    QString ipAddress;   ///< IP地址
    int port;            ///< 端口号
    bool isOnline;       ///< 是否在线
};

/**
 * @class CameraListWidget
 * @brief 相机列表控件类
 *
 * 提供相机列表的显示、管理和搜索功能，支持按名称、ID、类型进行过滤。
 */
class CameraListWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父控件指针
     */
    explicit CameraListWidget(QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~CameraListWidget();

    /**
     * @brief 添加相机
     * @param camera 相机信息
     */
    void addCamera(const CameraInfo &camera);

    /**
     * @brief 移除相机
     * @param cameraId 相机ID
     */
    void removeCamera(const QString &cameraId);

    /**
     * @brief 更新相机状态
     * @param cameraId 相机ID
     * @param status 新状态
     */
    void updateCameraStatus(const QString &cameraId, const QString &status);

    /**
     * @brief 清除所有相机
     */
    void clearCameras();

    /**
     * @brief 获取相机数量
     * @return 相机数量
     */
    int cameraCount() const;

    /**
     * @brief 获取所有相机信息
     * @return 相机信息列表
     */
    QList<CameraInfo> getCameras() const;

    /**
     * @brief 获取指定相机信息
     * @param cameraId 相机ID
     * @return 相机信息
     */
    CameraInfo getCamera(const QString &cameraId) const;

signals:
    /**
     * @brief 相机选中信号
     * @param cameraId 相机ID
     */
    void cameraSelected(const QString &cameraId);

    /**
     * @brief 相机双击信号
     * @param cameraId 相机ID
     */
    void cameraDoubleClicked(const QString &cameraId);

    /**
     * @brief 刷新请求信号
     */
    void refreshRequested();

private slots:
    /**
     * @brief 项点击处理
     * @param item 项
     * @param column 列
     */
    void onItemClicked(QTreeWidgetItem *item, int column);

    /**
     * @brief 项双击处理
     * @param item 项
     * @param column 列
     */
    void onItemDoubleClicked(QTreeWidgetItem *item, int column);

    /**
     * @brief 刷新按钮点击处理
     */
    void onRefreshClicked();

    /**
     * @brief 搜索文本改变处理
     * @param text 搜索文本
     */
    void onSearchTextChanged(const QString &text);

    /**
     * @brief 项选择改变处理
     */
    void onItemSelectionChanged();

private:
    /**
     * @brief 设置用户界面
     */
    void setupUi();

    /**
     * @brief 设置信号槽连接
     */
    void setupConnections();

    /**
     * @brief 更新相机项
     * @param item 项
     * @param camera 相机信息
     */
    void updateCameraItem(QTreeWidgetItem *item, const CameraInfo &camera);

    /**
     * @brief 过滤相机
     * @param filter 过滤文本
     */
    void filterCameras(const QString &filter);

private:
    QVBoxLayout *m_mainLayout;                    // 主布局
    QHBoxLayout *m_searchLayout;                  // 搜索布局
    QLineEdit *m_searchEdit;                      // 搜索输入框
    QPushButton *m_refreshBtn;                    // 刷新按钮

    QLabel *m_titleLabel;                         // 标题标签
    QTreeWidget *m_cameraTree;                    // 相机树形列表

    QLabel *m_countLabel;                         // 计数标签

    QMap<QString, QTreeWidgetItem*> m_cameraItems; // 相机项映射
    QMap<QString, CameraInfo> m_cameras;          // 相机信息映射
};

} // namespace ground_station

#endif // GROUND_STATION_CAMERALISTWIDGET_H