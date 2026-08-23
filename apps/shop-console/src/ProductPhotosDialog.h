#pragma once

#include <QDialog>
#include <QString>

#include "SupabaseClient.h"

class QListWidget;
class QListWidgetItem;
class QLabel;
class QNetworkAccessManager;

// Manage the photos of a single, already-existing product: view thumbnails,
// add more (uploads straight to Storage via SupabaseClient), remove any.
// This is the only place photos can be touched after a product is first
// created — ProductDialog only handles photos at creation time.
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
