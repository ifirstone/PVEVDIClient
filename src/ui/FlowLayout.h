#ifndef FLOWLAYOUT_H
#define FLOWLAYOUT_H

#include <QLayout>
#include <QRect>
#include <QList>
#include <QStyle>

// 自适应流式布局：先横向排列控件，一行排满后自动换下一行
// 类似 CSS 的 flex-wrap: wrap 或 HTML 的 flow layout
class FlowLayout : public QLayout
{
public:
    explicit FlowLayout(QWidget *parent, int margin = 24, int hSpacing = 16, int vSpacing = 16);
    explicit FlowLayout(int margin = 24, int hSpacing = 16, int vSpacing = 16);
    ~FlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int horizontalSpacing() const;
    int verticalSpacing() const;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

private:
    int doLayout(const QRect &rect, bool testOnly) const;
    int smartSpacing(QStyle::PixelMetric pm) const;

    QList<QLayoutItem *> m_itemList;
    int m_hSpace;
    int m_vSpace;
};

#endif // FLOWLAYOUT_H
