#include "ProductPhotosDialog.h"

#include "Config.h"

#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QPushButton>
#include <QVBoxLayout>

ProductPhotosDialog::ProductPhotosDialog(SupabaseClient *client, QString productId,
                                          const QString &productName, QWidget *parent)
    : QDialog(parent), m_client(client), m_productId(std::move(productId)) {
    setWindowTitle("Photos — " + productName);
    resize(520, 420);

    m_list = new QListWidget(this);
    m_list->setViewMode(QListView::IconMode);
    m_list->setIconSize(QSize(96, 96));
    m_list->setResizeMode(QListView::Adjust);
    m_list->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_list->setSpacing(8);

    auto *addButton = new QPushButton("Add photos…", this);
    auto *removeButton = new QPushButton("Remove selected", this);
    auto *closeButton = new QPushButton("Close", this);
    auto *buttonRow = new QHBoxLayout;
    buttonRow->addWidget(addButton);
    buttonRow->addWidget(removeButton);
    buttonRow->addStretch();
    buttonRow->addWidget(closeButton);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setStyleSheet("color: #dc2626;");
    m_statusLabel->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_list, 1);
    layout->addWidget(m_statusLabel);
    layout->addLayout(buttonRow);

    connect(addButton, &QPushButton::clicked, this, &ProductPhotosDialog::addPhotos);
    connect(removeButton, &QPushButton::clicked, this, &ProductPhotosDialog::removeSelected);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);

    m_thumbnailNetwork = new QNetworkAccessManager(this);

    connect(m_client, &SupabaseClient::productImagesFetched, this, [this](const QVector<ProductImage> &images) {
        populateList(images);
    });
    connect(m_client, &SupabaseClient::productImagesFetchFailed, this, [this](const QString &message) {
        m_statusLabel->setText("Couldn't load photos: " + message);
    });
    connect(m_client, &SupabaseClient::imageUploaded, this, [this]() { refresh(); });
    connect(m_client, &SupabaseClient::imageUploadFailed, this, [this](const QString &message) {
        m_statusLabel->setText("Upload failed: " + message);
        refresh();
    });
    connect(m_client, &SupabaseClient::imageDeleted, this, [this]() { refresh(); });
    connect(m_client, &SupabaseClient::imageDeleteFailed, this, [this](const QString &message) {
        m_statusLabel->setText("Couldn't remove photo: " + message);
        refresh();
    });

    refresh();
}

void ProductPhotosDialog::refresh() {
    m_client->fetchProductImages(m_productId);
}

void ProductPhotosDialog::populateList(const QVector<ProductImage> &images) {
    ++m_generation;
    m_list->clear();

    for (const ProductImage &image : images) {
        auto *item = new QListWidgetItem(image.isPrimary ? "★ primary" : "");
        item->setData(Qt::UserRole, image.id);
        item->setData(Qt::UserRole + 1, image.storagePath);
        m_list->addItem(item);
        loadThumbnail(item, m_client->publicImageUrl(image.storagePath), m_generation);
    }

    m_statusLabel->setText(images.isEmpty() ? "No photos yet." : QString());
}

void ProductPhotosDialog::loadThumbnail(QListWidgetItem *item, const QString &url, int generation) {
    QNetworkReply *reply = m_thumbnailNetwork->get(QNetworkRequest(QUrl(url)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, item, generation]() {
        reply->deleteLater();
        // The list may have been rebuilt (refresh()) since this request
        // started, in which case `item` no longer exists — bail out rather
        // than touch freed memory.
        if (generation != m_generation) return;

        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) return;

        QPixmap pixmap;
        if (pixmap.loadFromData(data)) {
            item->setIcon(QIcon(pixmap.scaled(96, 96, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
        }
    });
}

void ProductPhotosDialog::addPhotos() {
    const QStringList files = QFileDialog::getOpenFileNames(
        this, "Select photos", Config::load().photosSyncDir, "Images (*.png *.jpg *.jpeg *.webp)");
    for (const QString &path : files) {
        m_client->uploadProductImage(m_productId, path, /*isPrimary=*/false);
    }
}

void ProductPhotosDialog::removeSelected() {
    const QList<QListWidgetItem *> selected = m_list->selectedItems();
    if (selected.isEmpty()) return;

    const auto confirm = QMessageBox::question(
        this, "Remove photos", QString("Remove %1 photo(s)? This can't be undone.").arg(selected.size()));
    if (confirm != QMessageBox::Yes) return;

    for (QListWidgetItem *item : selected) {
        m_client->deleteProductImage(item->data(Qt::UserRole).toString(),
                                      item->data(Qt::UserRole + 1).toString());
    }
}
