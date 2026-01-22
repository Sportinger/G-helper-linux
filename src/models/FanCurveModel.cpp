#include "FanCurveModel.h"

FanCurveModel::FanCurveModel(QObject *parent)
    : QAbstractListModel(parent)
{
    // Initialize with default 8 points
    loadDefaultCurve(1); // Balanced
}

FanCurveModel::~FanCurveModel() = default;

int FanCurveModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_points.size();
}

QVariant FanCurveModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_points.size())
        return QVariant();

    const FanCurvePointData &point = m_points.at(index.row());

    switch (role) {
        case TemperatureRole:
            return point.temperature;
        case FanPercentRole:
            return point.fanPercent;
        default:
            return QVariant();
    }
}

bool FanCurveModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() >= m_points.size())
        return false;

    FanCurvePointData &point = m_points[index.row()];

    switch (role) {
        case TemperatureRole:
            point.temperature = value.toInt();
            break;
        case FanPercentRole:
            point.fanPercent = qBound(0, value.toInt(), 100);
            break;
        default:
            return false;
    }

    emit dataChanged(index, index, {role});
    emit dataModified();
    return true;
}

QHash<int, QByteArray> FanCurveModel::roleNames() const
{
    return {
        {TemperatureRole, "temperature"},
        {FanPercentRole, "fanPercent"}
    };
}

void FanCurveModel::setPoint(int index, int temperature, int fanPercent)
{
    if (index < 0 || index >= m_points.size())
        return;

    m_points[index].temperature = temperature;
    m_points[index].fanPercent = qBound(0, fanPercent, 100);

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex, {TemperatureRole, FanPercentRole});
    emit dataModified();
}

int FanCurveModel::temperatureAt(int index) const
{
    if (index < 0 || index >= m_points.size())
        return 0;
    return m_points.at(index).temperature;
}

int FanCurveModel::fanPercentAt(int index) const
{
    if (index < 0 || index >= m_points.size())
        return 0;
    return m_points.at(index).fanPercent;
}

void FanCurveModel::loadFromVariantList(const QVariantList &points)
{
    beginResetModel();
    m_points.clear();

    for (const QVariant &pointVar : points) {
        QVariantMap pointMap = pointVar.toMap();
        FanCurvePointData point;
        point.temperature = pointMap.value("temp", 0).toInt();
        point.fanPercent = pointMap.value("fan", 0).toInt();
        m_points.append(point);
    }

    endResetModel();
    emit countChanged();
}

QVariantList FanCurveModel::toVariantList() const
{
    QVariantList result;

    for (const FanCurvePointData &point : m_points) {
        QVariantMap pointMap;
        pointMap["temp"] = point.temperature;
        pointMap["fan"] = point.fanPercent;
        result.append(pointMap);
    }

    return result;
}

void FanCurveModel::loadDefaultCurve(int profile)
{
    beginResetModel();
    m_points.clear();

    switch (profile) {
        case 0: // Quiet
            m_points = {
                {30, 0}, {45, 10}, {55, 25}, {65, 40},
                {75, 55}, {85, 70}, {95, 85}, {100, 100}
            };
            break;

        case 1: // Balanced
        default:
            m_points = {
                {30, 0}, {40, 15}, {50, 30}, {60, 45},
                {70, 60}, {80, 75}, {90, 90}, {100, 100}
            };
            break;

        case 2: // Performance
            m_points = {
                {30, 10}, {40, 25}, {50, 40}, {60, 55},
                {70, 70}, {80, 85}, {90, 95}, {100, 100}
            };
            break;
    }

    endResetModel();
    emit countChanged();
}

void FanCurveModel::clear()
{
    beginResetModel();
    m_points.clear();
    endResetModel();
    emit countChanged();
}
