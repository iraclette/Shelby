#pragma once

#include <QDialog>
#include <QString>

#include "SupabaseClient.h"

class QListWidget;
class QListWidgetItem;
class QLabel;
class QNetworkAccessManager;

// Add/remove photos of an existing product.
class ProductPhotosDialog : public QDialog {
    Q_OBJECT

public:
    ProductPhotosDialog(SupabaseClient *client, QString productId, const QString &productName,
                         QWidget *parent = nullptr);

private:
    void refresh();
    void populateList(const QVector<ProductImage> &images);
    void loadThumbnail(QListWidgetItem *item, const QString &url, int generation);
    void addPhotos();
    void removeSelected();

    SupabaseClient *m_client;
    QString m_productId;
    QListWidget *m_list;
    QLabel *m_statusLabel;
    QNetworkAccessManager *m_thumbnailNetwork;
    int m_generation = 0; // bumped on every list rebuild; guards against use-after-free
                           // when a thumbnail finishes downloading after a later refresh().
};
