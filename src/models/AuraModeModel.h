#ifndef AURAMODEMODEL_H
#define AURAMODEMODEL_H

#include <QAbstractListModel>
#include <QVector>

struct AuraModeData {
    int mode;
    QString name;
    QString icon;
    bool usesColor;
    bool usesTwoColors;
    bool usesSpeed;
};

class AuraModeModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count CONSTANT)

public:
    enum Roles {
        ModeRole = Qt::UserRole + 1,
        NameRole,
        IconRole,
        UsesColorRole,
        UsesTwoColorsRole,
        UsesSpeedRole
    };

    explicit AuraModeModel(QObject *parent = nullptr);
    ~AuraModeModel() override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const { return m_modes.size(); }

    Q_INVOKABLE int modeAt(int index) const;
    Q_INVOKABLE QString nameAt(int index) const;
    Q_INVOKABLE int indexOfMode(int mode) const;

private:
    void initModes();
    QVector<AuraModeData> m_modes;
};

#endif // AURAMODEMODEL_H
