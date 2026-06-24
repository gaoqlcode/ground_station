/**
 * @file cameralistwidget.cpp
 * @brief 相机列表界面控件实现
 *
 * 本文件实现了地面站客户端的相机列表管理界面控件。
 * 主要功能包括：
 * - 相机列表的显示和管理
 * - 相机状态实时更新
 * - 相机搜索过滤
 * - 相机选择和双击操作
 *
 * 使用QTreeWidget展示相机信息，支持按名称、ID、类型进行搜索过滤
 *
 * @author GroundStation Team
 * @date 2024
 */

#include "cameralistwidget.h"

#include <QHeaderView>
#include <QMessageBox>

namespace ground_station {

/**
 * @brief 构造函数，初始化相机列表控件
 * @param parent 父控件指针
 *
 * 初始化所有界面组件和数据结构
 */
CameraListWidget::CameraListWidget(QWidget *parent)
    : QWidget(parent)
    , m_mainLayout(nullptr)
    , m_searchLayout(nullptr)
    , m_searchEdit(nullptr)
    , m_refreshBtn(nullptr)
    , m_titleLabel(nullptr)
    , m_cameraTree(nullptr)
    , m_countLabel(nullptr)
{
    setupUi();
    setupConnections();
}

/**
 * @brief 析构函数
 */
CameraListWidget::~CameraListWidget()
{
}

/**
 * @brief 初始化用户界面
 *
 * 创建并布局所有界面组件，包括：
 * - 标题标签
 * - 搜索框和刷新按钮
 * - 相机树形列表
 * - 计数标签
 */
void CameraListWidget::setupUi()
{
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(5, 5, 5, 5);
    m_mainLayout->setSpacing(5);

    // 标题标签
    m_titleLabel = new QLabel(tr("相机列表"), this);
    m_titleLabel->setProperty("heading", true);
    m_mainLayout->addWidget(m_titleLabel);

    // 搜索框和刷新按钮的水平布局
    m_searchLayout = new QHBoxLayout();
    m_searchLayout->setSpacing(8);

    // 搜索输入框
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(tr("搜索相机..."));
    m_searchEdit->setClearButtonEnabled(true);

    // 刷新按钮
    m_refreshBtn = new QPushButton(tr("刷新"), this);
    m_refreshBtn->setMinimumWidth(80);

    m_searchLayout->addWidget(m_searchEdit);
    m_searchLayout->addWidget(m_refreshBtn);

    m_mainLayout->addLayout(m_searchLayout);

    // 相机树形列表控件
    m_cameraTree = new QTreeWidget(this);
    m_cameraTree->setHeaderLabels(QStringList() << tr("相机名称") << tr("状态") << tr("类型"));
    m_cameraTree->setRootIsDecorated(false);  // 不显示根节点装饰
    m_cameraTree->setAlternatingRowColors(true);  // 交替行颜色
    m_cameraTree->setSelectionMode(QAbstractItemView::SingleSelection);  // 单选模式
    m_cameraTree->setSortingEnabled(true);  // 启用排序
    m_cameraTree->sortByColumn(0, Qt::AscendingOrder);  // 按名称升序排序

    // 设置列宽调整模式
    QHeaderView *header = m_cameraTree->header();
    header->setStretchLastSection(true);
    header->setSectionResizeMode(0, QHeaderView::Stretch);  // 名称列自动拉伸
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);  // 状态列按内容调整
    header->setSectionResizeMode(2, QHeaderView::ResizeToContents);  // 类型列按内容调整

    m_mainLayout->addWidget(m_cameraTree);

    // 相机计数标签
    m_countLabel = new QLabel(tr("共 0 个相机"), this);
    m_mainLayout->addWidget(m_countLabel);
}

/**
 * @brief 建立信号槽连接
 *
 * 连接树形列表、搜索框和刷新按钮的信号到对应的槽函数
 */
void CameraListWidget::setupConnections()
{
    // 树形列表点击事件
    connect(m_cameraTree, &QTreeWidget::itemClicked,
            this, &CameraListWidget::onItemClicked);
    // 树形列表双击事件
    connect(m_cameraTree, &QTreeWidget::itemDoubleClicked,
            this, &CameraListWidget::onItemDoubleClicked);
    // 刷新按钮点击事件
    connect(m_refreshBtn, &QPushButton::clicked,
            this, &CameraListWidget::onRefreshClicked);
    // 搜索框文本变更事件
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &CameraListWidget::onSearchTextChanged);
    // 树形列表选择变更事件
    connect(m_cameraTree, &QTreeWidget::itemSelectionChanged,
            this, &CameraListWidget::onItemSelectionChanged);
}

/**
 * @brief 添加相机到列表
 * @param camera 相机信息结构体
 *
 * 如果相机已存在则更新其状态，否则添加新的列表项
 */
void CameraListWidget::addCamera(const CameraInfo &camera)
{
    // 检查相机是否已存在
    if (m_cameras.contains(camera.id)) {
        // 更新现有相机的状态
        updateCameraStatus(camera.id, camera.status);
        return;
    }

    // 创建新的树形列表项
    QTreeWidgetItem *item = new QTreeWidgetItem(m_cameraTree);
    updateCameraItem(item, camera);

    // 存储相机信息和列表项映射
    m_cameraItems[camera.id] = item;
    m_cameras[camera.id] = camera;

    // 更新计数标签
    m_countLabel->setText(tr("共 %1 个相机").arg(m_cameras.size()));
}

/**
 * @brief 从列表移除相机
 * @param cameraId 相机ID
 *
 * 移除指定ID的相机并更新计数
 */
void CameraListWidget::removeCamera(const QString &cameraId)
{
    // 检查相机是否存在
    if (!m_cameras.contains(cameraId)) {
        return;
    }

    // 从树形列表中移除项
    QTreeWidgetItem *item = m_cameraItems.take(cameraId);
    if (item) {
        int index = m_cameraTree->indexOfTopLevelItem(item);
        if (index >= 0) {
            m_cameraTree->takeTopLevelItem(index);
        }
        delete item;
    }

    // 从数据结构中移除
    m_cameras.remove(cameraId);
    m_countLabel->setText(tr("共 %1 个相机").arg(m_cameras.size()));
}

/**
 * @brief 更新相机状态
 * @param cameraId 相机ID
 * @param status 新状态字符串
 *
 * 更新相机状态并根据状态设置显示颜色：
 * - 在线：绿色
 * - 离线：红色
 * - 其他：黄色
 */
void CameraListWidget::updateCameraStatus(const QString &cameraId, const QString &status)
{
    // 检查相机是否存在
    if (!m_cameras.contains(cameraId)) {
        return;
    }

    // 更新数据结构中的状态
    m_cameras[cameraId].status = status;

    // 更新列表项显示
    QTreeWidgetItem *item = m_cameraItems.value(cameraId);
    if (item) {
        item->setText(1, status);

        // 根据状态设置前景颜色
        if (status == tr("在线") || status == "Online") {
            item->setForeground(1, QColor(0, 150, 0));  // 绿色
            m_cameras[cameraId].isOnline = true;
        } else if (status == tr("离线") || status == "Offline") {
            item->setForeground(1, QColor(150, 0, 0));  // 红色
            m_cameras[cameraId].isOnline = false;
        } else {
            item->setForeground(1, QColor(150, 150, 0));  // 黄色
            m_cameras[cameraId].isOnline = false;
        }
    }
}

/**
 * @brief 清空所有相机
 *
 * 清空树形列表和所有数据结构
 */
void CameraListWidget::clearCameras()
{
    m_cameraTree->clear();
    m_cameraItems.clear();
    m_cameras.clear();
    m_countLabel->setText(tr("共 0 个相机"));
}

/**
 * @brief 获取相机数量
 * @return 当前列表中的相机数量
 */
int CameraListWidget::cameraCount() const
{
    return m_cameras.size();
}

/**
 * @brief 获取所有相机列表
 * @return 相机信息列表
 */
QList<CameraInfo> CameraListWidget::getCameras() const
{
    return m_cameras.values();
}

/**
 * @brief 获取指定相机信息
 * @param cameraId 相机ID
 * @return 相机信息结构体
 */
CameraInfo CameraListWidget::getCamera(const QString &cameraId) const
{
    return m_cameras.value(cameraId);
}

/**
 * @brief 列表项点击槽函数
 * @param item 被点击的列表项
 * @param column 点击的列索引（未使用）
 *
 * 发射cameraSelected信号
 */
void CameraListWidget::onItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    // 从列表项数据中获取相机ID
    QString cameraId = item->data(0, Qt::UserRole).toString();
    emit cameraSelected(cameraId);
}

/**
 * @brief 列表项双击槽函数
 * @param item 被双击的列表项
 * @param column 双击的列索引（未使用）
 *
 * 发射cameraDoubleClicked信号
 */
void CameraListWidget::onItemDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    // 从列表项数据中获取相机ID
    QString cameraId = item->data(0, Qt::UserRole).toString();
    emit cameraDoubleClicked(cameraId);
}

/**
 * @brief 刷新按钮点击槽函数
 *
 * 发射refreshRequested信号请求刷新相机列表
 */
void CameraListWidget::onRefreshClicked()
{
    emit refreshRequested();
}

/**
 * @brief 搜索框文本变更槽函数
 * @param text 新的搜索文本
 *
 * 根据输入文本过滤相机列表
 */
void CameraListWidget::onSearchTextChanged(const QString &text)
{
    filterCameras(text);
}

/**
 * @brief 列表项选择变更槽函数
 *
 * 当选择项变更时发射cameraSelected信号
 */
void CameraListWidget::onItemSelectionChanged()
{
    QList<QTreeWidgetItem*> selectedItems = m_cameraTree->selectedItems();
    if (!selectedItems.isEmpty()) {
        QString cameraId = selectedItems.first()->data(0, Qt::UserRole).toString();
        emit cameraSelected(cameraId);
    }
}

/**
 * @brief 更新相机列表项显示
 * @param item 树形列表项指针
 * @param camera 相机信息结构体
 *
 * 设置列表项的文本、图标和工具提示
 */
void CameraListWidget::updateCameraItem(QTreeWidgetItem *item, const CameraInfo &camera)
{
    // 设置各列文本
    item->setText(0, camera.name);
    item->setText(1, camera.status);
    item->setText(2, camera.type);

    // 存储相机ID到列表项数据中
    item->setData(0, Qt::UserRole, camera.id);

    // 根据在线状态设置图标和颜色
    if (camera.isOnline) {
        item->setIcon(1, QIcon::fromTheme("user-online"));
        item->setForeground(1, QColor(0, 150, 0));  // 绿色
    } else {
        item->setIcon(1, QIcon::fromTheme("user-offline"));
        item->setForeground(1, QColor(150, 0, 0));  // 红色
    }

    // 设置工具提示，显示详细信息
    QString tooltip = tr("ID: %1\n名称: %2\n类型: %3\n状态: %4\n地址: %5:%6")
        .arg(camera.id)
        .arg(camera.name)
        .arg(camera.type)
        .arg(camera.status)
        .arg(camera.ipAddress)
        .arg(camera.port);
    item->setToolTip(0, tooltip);
}

/**
 * @brief 过滤相机列表
 * @param filter 过滤文本
 *
 * 根据过滤文本匹配相机名称、ID或类型，隐藏不匹配的项
 */
void CameraListWidget::filterCameras(const QString &filter)
{
    QString lowerFilter = filter.toLower();

    // 遍历所有相机项进行过滤
    for (auto it = m_cameraItems.begin(); it != m_cameraItems.end(); ++it) {
        QTreeWidgetItem *item = it.value();
        CameraInfo camera = m_cameras.value(it.key());

        // 检查是否匹配过滤条件（名称、ID或类型）
        bool match = filter.isEmpty() ||
                     camera.name.toLower().contains(lowerFilter) ||
                     camera.id.toLower().contains(lowerFilter) ||
                     camera.type.toLower().contains(lowerFilter);

        // 设置项的可见性
        item->setHidden(!match);
    }
}

} // namespace ground_station
