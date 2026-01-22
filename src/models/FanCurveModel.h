#ifndef FANCURVEMODEL_H
#define FANCURVEMODEL_H

#include <QAbstractListModel>
#include <QVector>

struct FanCurvePointData {
    int temperature;
    int fanPercent;
};

class FanCurveModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles {
        TemperatureRole = Qt::UserRole + 1,
        FanPercentRole
    };

    explicit FanCurveModel(QObject *parent = nullptr);
    ~FanCurveModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_points.size(); }

    Q_INVOKABLE void setPoint(int index, int temperature, int fanPercent);
    Q_INVOKABLE int temperatureAt(int index) const;
    Q_INVOKABLE int fanPercentAt(int index) const;
    Q_INVOKABLE void loadFromVariantList(const QVariantList &points);
    Q_INVOKABLE QVariantList toVariantList() const;
    Q_INVOKABLE void loadDefaultCurve(int profile);
    Q_INVOKABLE void clear();

signals:
    void countChanged();
    void dataModified();

private:
    QVector<FanCurvePointData> m_points;
};

#endif // FANCURVEMODEL_H
