#include "AuraModeModel.h"

AuraModeModel::AuraModeModel(QObject *parent)
    : QAbstractListModel(parent)
{
    initModes();
}

AuraModeModel::~AuraModeModel() = default;

void AuraModeModel::initModes()
{
    m_modes = {
        {255, tr("Off"), "qrc:/icons/led-off.svg", false, false, false},
        {0, tr("Static"), "qrc:/icons/led-static.svg", true, false, false},
        {1, tr("Breathe"), "qrc:/icons/led-breathe.svg", true, true, true},
        {2, tr("Strobe"), "qrc:/icons/led-strobe.svg", true, true, true},
        {3, tr("Rainbow"), "qrc:/icons/led-rainbow.svg", false, false, true},
        {9, tr("Pulse"), "qrc:/icons/led-pulse.svg", true, true, true},
        {10, tr("Comet"), "qrc:/icons/led-comet.svg", true, true, true}
    };
}

int AuraModeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_modes.size();
}

QVariant AuraModeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_modes.size())
        return QVariant();

    const AuraModeData &mode = m_modes.at(index.row());

    switch (role) {
        case ModeRole:
            return mode.mode;
        case NameRole:
            return mode.name;
        case IconRole:
            return mode.icon;
        case UsesColorRole:
            return mode.usesColor;
        case UsesTwoColorsRole:
            return mode.usesTwoColors;
        case UsesSpeedRole:
            return mode.usesSpeed;
        default:
            return QVariant();
    }
}

QHash<int, QByteArray> AuraModeModel::roleNames() const
{
    return {
        {ModeRole, "mode"},
        {NameRole, "name"},
        {IconRole, "icon"},
        {UsesColorRole, "usesColor"},
        {UsesTwoColorsRole, "usesTwoColors"},
        {UsesSpeedRole, "usesSpeed"}
    };
}

int AuraModeModel::modeAt(int index) const
{
    if (index < 0 || index >= m_modes.size())
        return -1;
    return m_modes.at(index).mode;
}

QString AuraModeModel::nameAt(int index) const
{
    if (index < 0 || index >= m_modes.size())
        return QString();
    return m_modes.at(index).name;
}

int AuraModeModel::indexOfMode(int mode) const
{
    for (int i = 0; i < m_modes.size(); ++i) {
        if (m_modes.at(i).mode == mode)
            return i;
    }
    return -1;
}
