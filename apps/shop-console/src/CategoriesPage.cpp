#include "CategoriesPage.h"
#include "SupabaseClient.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

CategoriesPage::CategoriesPage(SupabaseClient *client, QWidget *parent)
    : QWidget(parent), m_client(client) {
    auto *toolbar = new QHBoxLayout;
    auto *refreshButton = new QPushButton(tr("Refresh"), this);
    toolbar->addWidget(refreshButton);
    auto *newCategoryButton = new QPushButton(tr("New category"), this);
    toolbar->addWidget(newCategoryButton);
    toolbar->addStretch();

    connect(refreshButton, &QPushButton::clicked, this, [this]() { m_client->fetchCategories(); });
    connect(newCategoryButton, &QPushButton::clicked, this,
            [this]() { promptForNewCategory(this, m_client, m_categories); });

    m_table = new QTableWidget(this);
    m_table->setColumnCount(2);
    m_table->setHorizontalHeaderLabels({tr("Category"), tr("Visible on storefront")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(kColName, QHeaderView::ResizeToContents);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(toolbar);
    layout->addWidget(m_table);

    connect(m_table, &QTableWidget::itemChanged, this, &CategoriesPage::handleItemChanged);

    connect(m_client, &SupabaseClient::categoriesFetched, this, [this](const QVector<Category> &categories) {
        m_categories = categories;
        rebuildTable();
    });
    connect(m_client, &SupabaseClient::categoriesFetchFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Failed to load categories: ") + message);
    });
    m_client->fetchCategories();

    connect(m_client, &SupabaseClient::categoryUpdateFailed, this, [this](const QString &message) {
        emit statusMessage(tr("Update failed: ") + message);
        m_client->fetchCategories();
    });
    connect(m_client, &SupabaseClient::categoryUpdated, this, [this]() { m_client->fetchCategories(); });

    connect(m_client, &SupabaseClient::categoryCreated, this, [this]() { m_client->fetchCategories(); });
    connect(m_client, &SupabaseClient::categoryCreateFailed, this, [this](const QString &message) {
        QMessageBox::warning(this, tr("Couldn't create category"), message);
    });
}

void CategoriesPage::promptForNewCategory(QWidget *parent, SupabaseClient *client,
                                          const QVector<Category> &categories) {
    bool ok = false;
    const QString name = QInputDialog::getText(parent, tr("New category"), tr("Category name:"),
                                                 QLineEdit::Normal, {}, &ok);
    if (!ok || name.trimmed().isEmpty()) return;

    QStringList parentNames{tr("None (top-level category)")};
    QStringList parentIds{QString()};
    for (const Category &category : categories) {
        if (!category.parentId.isEmpty()) continue;
        parentNames.append(category.name);
        parentIds.append(category.id);
    }
    const QString picked = QInputDialog::getItem(parent, tr("New category"), tr("Subcategory of:"), parentNames, 0, false, &ok);
    if (!ok) return;

    client->createCategory(name.trimmed(), parentIds.value(parentNames.indexOf(picked)));
}

void CategoriesPage::rebuildTable() {
    m_populating = true;
    m_categories = categoriesInTreeOrder(m_categories);

    m_table->setRowCount(m_categories.size());
    for (int row = 0; row < m_categories.size(); ++row) {
        const Category &category = m_categories[row];

        // Subcategories sit indented under their parent.
        auto *nameItem = new QTableWidgetItem(category.parentId.isEmpty() ? category.name : "      " + category.name);
        nameItem->setData(Qt::UserRole, category.id);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(row, kColName, nameItem);

        auto *visibleItem = new QTableWidgetItem;
        visibleItem->setFlags((visibleItem->flags() & ~Qt::ItemIsEditable) | Qt::ItemIsUserCheckable);
        visibleItem->setCheckState(category.visibleOnStorefront ? Qt::Checked : Qt::Unchecked);
        m_table->setItem(row, kColVisible, visibleItem);
    }

    m_populating = false;
    emit statusMessage(tr("%n categories.", nullptr, int(m_categories.size())));
}

void CategoriesPage::handleItemChanged(QTableWidgetItem *item) {
    if (m_populating || item->column() != kColVisible) return;

    const int row = item->row();
    const QTableWidgetItem *nameItem = m_table->item(row, kColName);
    const QString categoryId = nameItem ? nameItem->data(Qt::UserRole).toString() : QString();
    if (categoryId.isEmpty()) return;

    m_client->updateCategoryField(categoryId, "visible_on_storefront", item->checkState() == Qt::Checked);
}
