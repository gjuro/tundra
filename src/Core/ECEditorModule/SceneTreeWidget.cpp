/**
    For conditions of distribution and use, see copyright notice in LICENSE

    @file   SceneTreeWidget.cpp
    @brief  Tree widget showing the scene structure. */

#include "StableHeaders.h"
#include "DebugOperatorNew.h"

#include "SceneTreeWidget.h"
#include "SceneTreeWidgetItems.h"
#include "SceneStructureModule.h"
#include "SupportedFileTypes.h"
#include "Scene.h"
#include "QtUtils.h"
#include "LoggingFunctions.h"
#include "SceneImporter.h"

#include "Entity.h"
#include "ConfigAPI.h"
#include "ECEditorWindow.h"
#include "ECEditorModule.h"
#include "EntityActionDialog.h"
#include "AddComponentDialog.h"
#include "NewEntityDialog.h"
#include "FunctionDialog.h"
#include "ArgumentType.h"
#include "InvokeItem.h"
#include "FunctionInvoker.h"
#include "CoreTypes.h"
#include "AssetAPI.h"
#include "SceneAPI.h"
#include "IAsset.h"
#include "IAssetTransfer.h"
#include "UiAPI.h"
#include "UiMainWindow.h"

#include "AssetCache.h"
#include "IAssetTypeFactory.h"

#include <QDomDocument>
#include <QDomElement>
#include <QDebug>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QDialog>

#include <kNet/DataSerializer.h>

#include "MemoryLeakCheck.h"

#ifdef Q_WS_MAC
#define KEY_DELETE_SHORTCUT QKeySequence(Qt::CTRL + Qt::Key_Backspace)
#else
#define KEY_DELETE_SHORTCUT QKeySequence::Delete
#endif

// Menu

Menu::Menu(QWidget *parent) : QMenu(parent), shiftDown(false)
{
}

void Menu::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Shift)
        shiftDown = true;
    QWidget::keyPressEvent(e);
}

void Menu::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Shift)
        shiftDown = false;
    QMenu::keyReleaseEvent(e);
}

// SceneTreeWidget

SceneTreeWidget::SceneTreeWidget(Framework *fw, QWidget *parent) :
    QTreeWidget(parent),
    framework(fw),
    showComponents(false),
    historyMaxItemCount(100),
    numberOfInvokeItemsVisible(5),
    fetchReferences(false)
{
    setEditTriggers(/*QAbstractItemView::EditKeyPressed*/QAbstractItemView::NoEditTriggers/*EditKeyPressed*/);
    setDragDropMode(QAbstractItemView::DropOnly/*DragDrop*/);
//    setDragEnabled(true);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSelectionBehavior(QAbstractItemView::SelectItems);
    setAnimated(true);
    setAllColumnsShowFocus(true);
    //setDefaultDropAction(Qt::MoveAction);
    setDropIndicatorShown(true);
    setHeaderLabel(tr("Scene entities"));
    setColumnCount(2);
    header()->setSectionHidden(1, true);

    connect(this, SIGNAL(doubleClicked(const QModelIndex &)), SLOT(Edit()));

    // Create keyboard shortcuts.
    QShortcut *renameShortcut = new QShortcut(QKeySequence(Qt::Key_F2), this);
    QShortcut *deleteShortcut = new QShortcut(KEY_DELETE_SHORTCUT, this);
    QShortcut *copyShortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_C), this);
    QShortcut *pasteShortcut = new QShortcut(QKeySequence(Qt::ControlModifier + Qt::Key_V), this);

    connect(renameShortcut, SIGNAL(activated()), SLOT(Rename()));
    connect(deleteShortcut, SIGNAL(activated()), SLOT(Delete()));
    connect(copyShortcut, SIGNAL(activated()), SLOT(Copy()));
    connect(pasteShortcut, SIGNAL(activated()), SLOT(Paste()));

//    disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnItemEdited(QTreeWidgetItem *, int)));

    LoadInvokeHistory();
}

SceneTreeWidget::~SceneTreeWidget()
{
    while(!ecEditors.empty())
    {
        ECEditorWindow *editor = ecEditors.back();
        ecEditors.pop_back();
        if (editor)
            SAFE_DELETE(editor);
    }
    if (fileDialog)
        fileDialog->close();

    SaveInvokeHistory();
}

void SceneTreeWidget::SetScene(const ScenePtr &s)
{
    scene = s;
}

void SceneTreeWidget::contextMenuEvent(QContextMenuEvent *e)
{
    // Do mousePressEvent so that the right item gets selected before we show the menu
    // (right-click doesn't do this automatically).
    QMouseEvent mouseEvent(QEvent::MouseButtonPress, e->pos(), e->globalPos(),
        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);

    mousePressEvent(&mouseEvent);

    // Create context menu and show it.
    SAFE_DELETE(contextMenu);
    contextMenu = new Menu(this);

    AddAvailableActions(contextMenu);

    contextMenu->popup(e->globalPos());
}

void SceneTreeWidget::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        foreach (QUrl url, e->mimeData()->urls())
            if (SceneStructureModule::IsSupportedFileType(url.path()))
                e->accept();
    }
    else
        QWidget::dragEnterEvent(e);
}

void SceneTreeWidget::dragMoveEvent(QDragMoveEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        foreach(QUrl url, e->mimeData()->urls())
            if (SceneStructureModule::IsSupportedFileType(url.path()))
                e->accept();
    }
    else
        QWidget::dragMoveEvent(e);
}

void SceneTreeWidget::dropEvent(QDropEvent *e)
{
    const QMimeData *data = e->mimeData();
    if (data->hasUrls())
    {
        SceneStructureModule *sceneStruct = framework->GetModule<SceneStructureModule>();
        if (!sceneStruct)
            LogError("Could not retrieve SceneStructureModule. Cannot instantiate content.");

        foreach(QUrl url, data->urls())
        {
            QString filename = url.path();
#ifdef _WINDOWS
            // We have '/' as the first char on windows and the filename
            // is not identified as a file properly. But on other platforms the '/' is valid/required.
            filename = filename.mid(1);
#endif
            if (SceneStructureModule::IsSupportedFileType(filename))
                if (sceneStruct)
                    sceneStruct->InstantiateContent(filename, float3::zero, false);
        }

        e->acceptProposedAction();
    }
    else
        QTreeWidget::dropEvent(e);
}

void SceneTreeWidget::AddAvailableActions(QMenu *menu)
{
    assert(menu);

    SceneTreeWidgetSelection sel = SelectedItems();

    if (sel.HasAssets() && !sel.HasComponents() && !sel.HasEntities())
        AddAvailableAssetActions(menu);
    else
        AddAvailableEntityActions(menu);
}

void SceneTreeWidget::AddAvailableAssetActions(QMenu *menu)
{
    assert(menu);

    //SceneTreeWidgetSelection sel = SelectedItems();
    AssetTreeWidgetSelection sel = SelectedItems();

    if (sel.HasAssets())
    {
        QMenu *deleteMenu = new QMenu(tr("Delete"), menu);
        QAction *deleteSourceAction = new QAction(tr("Delete from source"), deleteMenu);
        QAction *deleteCacheAction = new QAction(tr("Delete from cache"), deleteMenu);
        QAction *forgetAction = new QAction(tr("Forget asset"), deleteMenu);

        deleteMenu->addAction(deleteSourceAction);
        deleteMenu->addAction(deleteCacheAction);
        deleteMenu->addAction(forgetAction);
        menu->addMenu(deleteMenu);

        connect(deleteSourceAction, SIGNAL(triggered()), SLOT(DeleteFromSource()));
        connect(deleteCacheAction, SIGNAL(triggered()), SLOT(DeleteFromCache()));
        connect(forgetAction, SIGNAL(triggered()), SLOT(Forget()));

        QMenu *reloadMenu = new QMenu(tr("Reload"), menu);
        QAction *reloadFromSourceAction = new QAction(tr("Reload from source"), deleteMenu);
        QAction *reloadFromCacheAction = new QAction(tr("Reload from cache"), deleteMenu);
        QAction *unloadAction = new QAction(tr("Unload"), deleteMenu);

        // Reload from cache & delete from cache are not possible for e.g. local assets don't have a cached version of the asset,
        // Even if the asset is an HTTP asset, these options are disable if there does not exist a cached version of that asset in the cache.
        foreach(AssetItem *item, sel.assets)
            if (item->Asset() && framework->Asset()->GetAssetCache()->FindInCache(item->Asset()->Name()).isEmpty())
            {
                reloadFromCacheAction->setDisabled(true);
                deleteCacheAction->setDisabled(true);
                break;
            }

        reloadMenu->addAction(reloadFromSourceAction);
        reloadMenu->addAction(reloadFromCacheAction);
        reloadMenu->addAction(unloadAction);
        menu->addMenu(reloadMenu);

        connect(reloadFromSourceAction, SIGNAL(triggered()), SLOT(ReloadFromSource()));
        connect(reloadFromCacheAction, SIGNAL(triggered()), SLOT(ReloadFromCache()));
        connect(unloadAction, SIGNAL(triggered()), SLOT(Unload()));

        QAction *openFileLocationAction = new QAction(tr("Open file location"), menu);
        menu->addAction(openFileLocationAction);
        connect(openFileLocationAction, SIGNAL(triggered()), SLOT(OpenFileLocation()));

        QAction *openInExternalEditor = new QAction(tr("Open in external editor"), menu);
        menu->addAction(openInExternalEditor);
        connect(openInExternalEditor, SIGNAL(triggered()), SLOT(OpenInExternalEditor()));

        // Delete from Source, Delete from Cache, Reload from Source, Unload, Open File Location, and Open in external editor
        // are not applicable for assets which have been created programmatically (disk source is empty).
        ///\todo Currently disk source is empty for unloaded assets, and open file location is disabled for them. This should not happen.
        foreach(AssetItem *item, sel.assets)
            if (item->Asset() && item->Asset()->DiskSource().trimmed().isEmpty())
            {
                deleteCacheAction->setDisabled(true);
                // If asset is an external URL, do not disable deleteFromSource & reloadFromSource
                if (AssetAPI::ParseAssetRef(item->Asset()->Name()) != AssetAPI::AssetRefExternalUrl)
                {
                    deleteSourceAction->setDisabled(true);
                    reloadFromSourceAction->setDisabled(true);
                }
                unloadAction->setDisabled(true);
                openFileLocationAction->setDisabled(true);
                openInExternalEditor->setDisabled(true);
                
                break;
            }

        menu->addSeparator();

        if (sel.assets.count() == 1)
        {
            QAction *copyAssetRefAction = new QAction(tr("Copy asset reference to clipboard"), menu);
            menu->addAction(copyAssetRefAction);
            connect(copyAssetRefAction, SIGNAL(triggered()), SLOT(CopyAssetRef()));
        }

        QAction *cloneAction = new QAction(tr("Clone..."), menu);
        menu->addAction(cloneAction);
        connect(cloneAction, SIGNAL(triggered()), SLOT(Clone()));

        QAction *exportAction = new QAction(tr("Export..."), menu);
        menu->addAction(exportAction);
        connect(exportAction, SIGNAL(triggered()), SLOT(Export()));
    }

    QAction *functionsAction = new QAction(tr("Functions..."), menu);
    connect(functionsAction, SIGNAL(triggered()), SLOT(OpenFunctionDialog()));
    menu->addAction(functionsAction);
    // "Functions..." is disabled if we have both assets and storages selected simultaneously.
    if (sel.HasAssets())// && sel.HasStorages())
        functionsAction->setDisabled(true);

    QAction *importAction = new QAction(tr("Import..."), menu);
    connect(importAction, SIGNAL(triggered()), SLOT(Import()));
    menu->addAction(importAction);

    QAction *requestNewAssetAction = new QAction(tr("Request new asset..."), menu);
    connect(requestNewAssetAction, SIGNAL(triggered()), SLOT(RequestNewAsset()));
    menu->addAction(requestNewAssetAction);

    QMenu *createMenu = new QMenu(tr("Create"), menu);
    menu->addMenu(createMenu);
    foreach(const AssetTypeFactoryPtr &factory, framework->Asset()->GetAssetTypeFactories())
    {
        QAction *createAsset = new QAction(factory->Type(), createMenu);
        createAsset->setObjectName(factory->Type());
        connect(createAsset, SIGNAL(triggered()), SLOT(CreateAsset()));
        createMenu->addAction(createAsset);
    }

    if (sel.storages.count() == 1)
    {
        QAction *makeDefaultStorageAction = new QAction(tr("Make default storage"), menu);
        connect(makeDefaultStorageAction, SIGNAL(triggered()), SLOT(MakeDefaultStorage()));
        menu->addAction(makeDefaultStorageAction);
    }

    if (sel.storages.count() > 0)
    {
        QAction *removeStorageAction = new QAction(tr("Remove storage"), menu);
        connect(removeStorageAction, SIGNAL(triggered()), SLOT(RemoveStorage()));
        menu->addAction(removeStorageAction);
    }

    // Let other instances add their possible functionality.
    // For now, pass only asset items.
    QList<QObject *> targets;
    foreach(AssetItem *item, sel.assets)
        targets.append(item->Asset().get());
    framework->Ui()->EmitContextMenuAboutToOpen(menu, targets);
    /*
    // Let other instances add their possible functionality.
    QList<QObject *> targets;
    foreach(AssetRefItem *item, sel.assets)
    {
        AssetPtr asset = framework->Asset()->GetAsset(item->id);
        if (asset)
            targets.append(asset.get());
    }

    if (targets.size())
        framework->Ui()->EmitContextMenuAboutToOpen(menu, targets);

    QAction *action;
    if (sel.assets.size() > 0)
    {
        if (sel.assets.size() > 1)
            action = new QAction(tr("Save selected assets..."), menu);
        else
        {
            QString assetName = sel.assets[0]->id.right(sel.assets[0]->id.size() - sel.assets[0]->id.lastIndexOf("://") - 3);
            action = new QAction(tr("Save") + " " + assetName + " " + tr("as..."), menu);
        }
        connect(action, SIGNAL(triggered()), SLOT(SaveAssetAs()));
        menu->addAction(action);
    }
    */
}

void SceneTreeWidget::AddAvailableEntityActions(QMenu *menu)
{
    assert(menu);

    // "New entity...", "Import..." and "Open new scene..." actions are available always
    QAction *newEntityAction = new QAction(tr("New entity..."), menu);
    QAction *importAction = new QAction(tr("Import..."), menu);
    QAction *openNewSceneAction = new QAction(tr("Open new scene..."), menu);

    connect(newEntityAction, SIGNAL(triggered()), SLOT(NewEntity()));
    connect(importAction, SIGNAL(triggered()), SLOT(Import()));
    connect(openNewSceneAction, SIGNAL(triggered()), SLOT(OpenNewScene()));

    // "Paste" action is available only if we have valid entity-component XML data in clipboard.
    QAction *pasteAction = 0;
    bool pastePossible = false;
    {
        QDomDocument sceneDoc("Scene");
        pastePossible = sceneDoc.setContent(QApplication::clipboard()->text());
        if (pastePossible)
        {
            pasteAction = new QAction(tr("Paste"), menu);
            connect(pasteAction, SIGNAL(triggered()), SLOT(Paste()));
        }
    }

    // "Save scene as..." action is possible if we have at least one entity in the scene.
    bool saveSceneAsPossible = (topLevelItemCount() > 0);
    QAction *saveSceneAsAction = 0;
    QAction *exportAllAction = 0;
    if (saveSceneAsPossible)
    {
        saveSceneAsAction = new QAction(tr("Save scene as..."), menu);
        connect(saveSceneAsAction, SIGNAL(triggered()), SLOT(SaveSceneAs()));

        exportAllAction = new QAction(tr("Export..."), menu);
        connect(exportAllAction, SIGNAL(triggered()), SLOT(ExportAll()));
    }

    // "Edit", "Edit in new", "New component...", "Delete", "Copy", "Actions..." and "Functions..."
    // "Convert to local", "Convert to replicated", and "Temporary" actions are available only if we have selection.
    QAction *editAction = 0, *editInNewAction = 0, *newComponentAction = 0, *deleteAction = 0,
        *renameAction = 0, *copyAction = 0, *saveAsAction = 0, *actionsAction = 0, *functionsAction = 0,
        *toLocalAction = 0, *toReplicatedAction = 0, *temporaryAction = 0;

    bool hasSelection = !selectionModel()->selection().isEmpty();
    if (hasSelection)
    {
        editAction = new QAction(tr("Edit"), menu);
        editInNewAction = new QAction(tr("Edit in new window"), menu);
        newComponentAction = new QAction(tr("New component..."), menu);
        deleteAction = new QAction(tr("Delete"), menu);
        copyAction = new QAction(tr("Copy"), menu);
        toLocalAction = new QAction(tr("Convert to local"), menu);
        toReplicatedAction = new QAction(tr("Convert to replicated"), menu);
        temporaryAction = new QAction(tr("Temporary"), menu);
        temporaryAction->setCheckable(true);
        temporaryAction->setChecked(false);
        saveAsAction = new QAction(tr("Save as..."), menu);
        actionsAction = new QAction(tr("Actions..."), menu);
        functionsAction = new QAction(tr("Functions..."), menu);

        connect(editAction, SIGNAL(triggered()), SLOT(Edit()));
        connect(editInNewAction, SIGNAL(triggered()), SLOT(EditInNew()));
        connect(newComponentAction, SIGNAL(triggered()), SLOT(NewComponent()));
        connect(deleteAction, SIGNAL(triggered()), SLOT(Delete()));
        connect(copyAction, SIGNAL(triggered()), SLOT(Copy()));
        connect(saveAsAction, SIGNAL(triggered()), SLOT(SaveAs()));
        connect(actionsAction, SIGNAL(triggered()), SLOT(OpenEntityActionDialog()));
        connect(functionsAction, SIGNAL(triggered()), SLOT(OpenFunctionDialog()));
        connect(toLocalAction, SIGNAL(triggered()), SLOT(ConvertEntityToLocal()));
        connect(toReplicatedAction, SIGNAL(triggered()), SLOT(ConvertEntityToReplicated()));
    }

    // "Rename" action is possible only if have one entity selected.
    bool renamePossible = (selectionModel()->selectedIndexes().size() == 1);
    if (renamePossible)
    {
        renameAction = new QAction(tr("Rename"), menu);
        connect(renameAction, SIGNAL(triggered()), SLOT(Rename()));
    }

    if (renamePossible)
        menu->addAction(renameAction);

    if (hasSelection)
    {
        menu->addAction(editAction);
        menu->setDefaultAction(editAction);
        menu->addAction(editInNewAction);
    }

    menu->addAction(newEntityAction);

    SceneTreeWidgetSelection sel = SelectedItems();

    if (hasSelection)
    {
        menu->addAction(newComponentAction);
        menu->addAction(deleteAction);
        menu->addAction(copyAction);
        menu->addAction(toLocalAction);
        menu->addAction(toReplicatedAction);
        menu->addAction(temporaryAction);

        // Altering temporary, local and replicated properties is only possible if we have only entities selected
        // and if all the entites have currently the same state.
        if (!sel.HasComponents() && sel.HasEntities() && sel.entities.first()->Entity())
        {
            bool firstStateLocal = sel.entities.first()->Entity()->IsLocal();
            bool firstStateReplicated = !firstStateLocal; // Entity is always either local or replicated.
            bool localMismatch = false;
            bool replicatedMismatch = false;
            bool firstStateTemporary = sel.entities.first()->Entity()->IsTemporary();
            if (sel.entities.size() > 1)
            {
                for(uint i = 1; i < (uint)sel.entities.size(); ++i)
                    if (sel.entities[i]->Entity())
                        if (firstStateLocal != sel.entities[i]->Entity()->IsLocal())
                        {
                            toLocalAction->setDisabled(true);
                            localMismatch = true;
                            break;
                        }
                for(uint i = 1; i < (uint)sel.entities.size(); ++i)
                    if (sel.entities[i]->Entity())
                        if (firstStateReplicated != sel.entities[i]->Entity()->IsReplicated())
                        {
                            toReplicatedAction->setDisabled(true);
                            replicatedMismatch = true;
                            break;
                        }
                for(uint i = 1; i < (uint)sel.entities.size(); ++i)
                    if (sel.entities[i]->Entity())
                        if (firstStateTemporary != sel.entities[i]->Entity()->IsTemporary())
                        {
                            temporaryAction->setDisabled(true);
                            break;
                        }
            }

            toLocalAction->setEnabled(localMismatch && replicatedMismatch ? false : !firstStateLocal);
            toReplicatedAction->setEnabled(localMismatch && replicatedMismatch ? false : !firstStateReplicated);
            temporaryAction->setChecked(firstStateTemporary);
        }
        else
        {
            toLocalAction->setDisabled(true);
            toReplicatedAction->setDisabled(true);
            temporaryAction->setDisabled(true);
        }
    }

    if (temporaryAction)
        connect(temporaryAction, SIGNAL(toggled(bool)), SLOT(SetAsTemporary(bool)));

    if (pastePossible)
        menu->addAction(pasteAction);

    menu->addSeparator();

    if (hasSelection)
        menu->addAction(saveAsAction);

    if (saveSceneAsPossible)
        menu->addAction(saveSceneAsAction);

    if (exportAllAction)
        menu->addAction(exportAllAction);

    menu->addAction(importAction);
    menu->addAction(openNewSceneAction);

    if (hasSelection)
    {
        menu->addSeparator();
        menu->addAction(actionsAction);
        menu->addAction(functionsAction);

        // "Functions..." is disabled if we have both entities and components selected simultaneously.
        if (sel.HasEntities() && sel.HasComponents())
            functionsAction->setDisabled(true);

        // Show only function and actions that for possible whole selection.
        QSet<QString> objectNames;
        foreach(EntityItem *eItem, sel.entities)
            if (eItem->Entity())
                objectNames << eItem->Entity().get()->metaObject()->className();
        foreach(ComponentItem *cItem, sel.components)
            if (cItem->Component())
                objectNames << cItem->Component().get()->metaObject()->className();

        // If we have more than one object name in the object name set, don't show the history.
        if (objectNames.size() == 1 && invokeHistory.size())
        {
            menu->addSeparator();

            int numItemsAdded = 0;
            for(int i = 0; i < invokeHistory.size(); ++i)
                if ((invokeHistory[i].objectName == objectNames.toList().first()) && (numItemsAdded < numberOfInvokeItemsVisible))
                {
                    QAction *invokeAction = new QAction(invokeHistory[i].ToString(), menu);
                    menu->addAction(invokeAction);
                    connect(invokeAction, SIGNAL(triggered()), SLOT(InvokeActionTriggered()));
                    ++numItemsAdded;
                }
        }
    }
    
    // Finally let others add functionality
    QList<QObject*> targets;
    if (hasSelection)
    {
        if (sel.HasEntities())
        {
            foreach(EntityItem* eItem, sel.entities)
                if (eItem->Entity())
                    targets.push_back(eItem->Entity().get());
        }
        if (sel.HasComponents())
        {
            foreach(ComponentItem* cItem, sel.components)
                if (cItem->Component())
                    targets.push_back(cItem->Component().get());
        }
    }
    framework->Ui()->EmitContextMenuAboutToOpen(menu, targets);
}

SceneTreeWidgetSelection SceneTreeWidget::SelectedItems() const
{
    SceneTreeWidgetSelection ret;
    QListIterator<QTreeWidgetItem *> it(selectedItems());
    while(it.hasNext())
    {
        QTreeWidgetItem *item = it.next();
        EntityItem *eItem = dynamic_cast<EntityItem *>(item);
        if (eItem)
            ret.entities << eItem;
        else
        {
            ComponentItem *cItem = dynamic_cast<ComponentItem *>(item);
            if (cItem)
                ret.components << cItem;
            else
            {
                AssetRefItem *aItem = dynamic_cast<AssetRefItem *>(item);
                if (aItem)
                    ret.assets << aItem;
            }
        }
    }

    return ret;
}

QString SceneTreeWidget::SelectionAsXml() const
{
    SceneTreeWidgetSelection selection = SelectedItems();
    if (selection.IsEmpty())
        return QString();

    // Create root Scene element always for consistency, even if we only have one entity
    QDomDocument sceneDoc("Scene");
    QDomElement sceneElem = sceneDoc.createElement("scene");

    if (selection.HasEntities())
    {
        foreach(EntityItem *eItem, selection.entities)
        {
            EntityPtr entity = eItem->Entity();
            assert(entity);
            if (entity)
            {
                if (!entity->IsTemporary())
                    entity->SerializeToXML(sceneDoc, sceneElem);
                else  //Cannot use Entity::SerializeToXML as it wont return temporary entities, which we want to include in the copy/paste feature.
                {
                    QDomElement entity_elem = sceneDoc.createElement("entity");

                    QString id_str;
                    id_str.setNum((int)entity->Id());
                    entity_elem.setAttribute("id", id_str);
                    entity_elem.setAttribute("sync", QString::fromStdString(::ToString<bool>(entity->IsReplicated())));
                    // Set a non-standard "temporary" property as we need to know in the paste step if the entity/component was temporary
                    entity_elem.setAttribute("temporary", QString::fromStdString(::ToString<bool>(entity->IsTemporary())));

                    const Entity::ComponentMap &components = entity->Components();
                    for (Entity::ComponentMap::const_iterator i = components.begin(); i != components.end(); ++i)
                        i->second->SerializeTo(sceneDoc, entity_elem);

                    sceneElem.appendChild(entity_elem);
                }
            }
        }

        sceneDoc.appendChild(sceneElem);
    }
    else if (selection.HasComponents())
    {
        foreach(ComponentItem *cItem, selection.components)
        {
            ComponentPtr component = cItem->Component();
            if (component)
                component->SerializeTo(sceneDoc, sceneElem);
        }

        sceneDoc.appendChild(sceneElem);
    }

    return sceneDoc.toString();
}

void SceneTreeWidget::LoadInvokeHistory()
{
    invokeHistory.clear();

    // Load and parse invoke history from settings.
    int idx = 0;
    forever
    {
        QString setting = framework->Config()->Get("uimemory", "invoke history", QString("item%1").arg(idx), "").toString();
        if (setting.isEmpty())
            break;
        invokeHistory.push_back(InvokeItem(setting));
        ++idx;
    }

    // Set MRU order numbers.
    for(int i = 0; i < invokeHistory.size(); ++i)
        invokeHistory[i].mruOrder = invokeHistory.size() - i;
}

void SceneTreeWidget::SaveInvokeHistory()
{
    // Sort descending by MRU order.
    qSort(invokeHistory.begin(), invokeHistory.end(), qGreater<InvokeItem>());
    for(int idx = 0; idx < invokeHistory.size(); ++idx)
        framework->Config()->Set("uimemory", "invoke history", QString("item%1").arg(idx), invokeHistory[idx].ToSetting());
}

InvokeItem *SceneTreeWidget::FindMruItem()
{
    InvokeItem *mruItem = 0;

    for(int i = 0; i< invokeHistory.size(); ++i)
    {
        if (mruItem == 0)
            mruItem = &invokeHistory[i];
        if (invokeHistory[i] > *mruItem)
            mruItem = &invokeHistory[i];
    }

    return mruItem;
}

void SceneTreeWidget::Edit()
{
    SceneTreeWidgetSelection selection = SelectedItems();
    if (selection.IsEmpty())
        return;

    if (selection.HasComponents() || selection.HasEntities())
    {
        if (ecEditors.size()) // If we have an existing editor instance, use it.
        {
            ECEditorWindow *editor = ecEditors.back();
            if (editor)
            {
                editor->AddEntities(selection.EntityIds(), true);
                editor->show();
                editor->activateWindow();
                return;
            }
        }

        // Check for active editor
        ECEditorWindow *editor = framework->GetModule<ECEditorModule>()->ActiveEditor();
        if (editor && !ecEditors.contains(editor))
        {
            editor->setAttribute(Qt::WA_DeleteOnClose);
            ecEditors.push_back(editor);
        }
        else // If there isn't any active editors in ECEditorModule, create a new one.
        {
            editor = new ECEditorWindow(framework, framework->Ui()->MainWindow());
            editor->setAttribute(Qt::WA_DeleteOnClose);
            editor->setWindowFlags(Qt::Tool);
            ecEditors.push_back(editor);
        }
        // To ensure that destroyed editors will get erased from the ecEditors list.
        connect(editor, SIGNAL(destroyed(QObject *)), SLOT(HandleECEditorDestroyed(QObject *)), Qt::UniqueConnection);

        //ecEditor->move(mapToGlobal(pos()) + QPoint(50, 50));
        //ecEditor->hide();

        if (!editor->isVisible())
        {
            editor->show();
            editor->activateWindow();
        }

        editor->AddEntities(selection.EntityIds(), true);
    }
}

void SceneTreeWidget::EditInNew()
{
    SceneTreeWidgetSelection selection = SelectedItems();
    if (selection.IsEmpty())
        return;

    // Create new editor instance every time, but if our "singleton" editor is not instantiated, create it.
    ECEditorWindow *editor = new ECEditorWindow(framework, framework->Ui()->MainWindow());
    editor->setAttribute(Qt::WA_DeleteOnClose);
    editor->setWindowFlags(Qt::Tool);
    connect(editor, SIGNAL(destroyed(QObject *)), SLOT(HandleECEditorDestroyed(QObject *)), Qt::UniqueConnection);
    //editor->move(mapToGlobal(pos()) + QPoint(50, 50));
    editor->hide();
    editor->AddEntities(selection.EntityIds(), true);

    /// \note Calling show and activate here makes the editor emit FocusChanged(this) twice in a row.
    editor->show();
    editor->activateWindow();
    ecEditors.push_back(editor);
}

void SceneTreeWidget::Rename()
{
    QModelIndex index = selectionModel()->currentIndex();
    if (!index.isValid())
        return;

    SceneTreeWidgetSelection sel = SelectedItems();
    if (sel.entities.size() == 1)
    {
        EntityItem *eItem = sel.entities[0];
        EntityPtr entity = eItem->Entity();
        if (entity)
        {
            // Disable sorting when renaming so that the item will not jump around in the list.
            setSortingEnabled(false);
            // Remove the entity ID from the text when user is editing entity's name.
            eItem->setText(0, entity->Name());
//            openPersistentEditor(eItem);
//            setCurrentIndex(index);
            edit(index);
            connect(this->itemDelegate(), SIGNAL(commitData(QWidget*)), SLOT(OnCommitData(QWidget*)), Qt::UniqueConnection);
            connect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)), SLOT(OnItemEdited(QTreeWidgetItem *, int)), Qt::UniqueConnection);
//            connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), SLOT(CloseEditor(QTreeWidgetItem *,QTreeWidgetItem *)));
        }
    }
/*
    else if (sel.components.size() == 1)
    {
        ComponentItem *cItem = sel.components[0];
        // Remove the type name from the text when user is editing entity's name.
        cItem->setText(0, cItem->name);
        edit(index);
//        connect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)), SLOT(OnItemEdited(QTreeWidgetItem *)), Qt::UniqueConnection);
    }
*/
}

void SceneTreeWidget::OnCommitData(QWidget * editor)
{
    QModelIndex index = selectionModel()->currentIndex();
    if (!index.isValid())
        return;

    SceneTreeWidgetSelection selection = SelectedItems();
    if (selection.entities.size() == 1)
    {
        EntityItem *entityItem = selection.entities[0];
        EntityPtr entity = entityItem->Entity();
        if (entity)
        {
            QLineEdit *edit = dynamic_cast<QLineEdit*>(editor);
            if (edit)
                // If there are no changes, restore back the previous stripped entity ID from the item, and restore sorting
                if (edit->text() == entity->Name())
                {
                    disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnItemEdited(QTreeWidgetItem *, int)));
                    entityItem->SetText(entity.get());
                    setSortingEnabled(true);
                }
        }
    }
}

void SceneTreeWidget::OnItemEdited(QTreeWidgetItem *item, int column)
{
    if (column != 0)
        return;

    EntityItem *eItem = dynamic_cast<EntityItem *>(item);
    if (eItem)
    {
        EntityPtr entity = eItem->Entity();
        assert(entity);
        if (entity)
        {
            QString newName = eItem->text(0);
            disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnItemEdited(QTreeWidgetItem *, int)));
            // We don't need to set item text here. It's done when SceneStructureWindow gets AttributeChanged() signal from Scene.
            entity->SetName(newName);
//            closePersistentEditor(item);
            setSortingEnabled(true);
            return;
        }
    }
/*
    ComponentItem *cItem = dynamic_cast<ComponentItem *>(item);
    EntityItem *parentItem = dynamic_cast<EntityItem *>(cItem->parent());
    if (cItem && parentItem)
    {
        EntityPtr entity = scene->GetEntity(parentItem->id);
        QString newName = cItem->text(0);
        disconnect(this, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(OnItemEdited(QTreeWidgetItem *)));
        ComponentPtr component = entity->GetComponent(cItem->typeName, cItem->name);
        if (component)
            component->SetName(newName);
        //cItem->typeName
    }
*/
}

/*
void SceneTreeWidget::CloseEditor(QTreeWidgetItem *c, QTreeWidgetItem *p)
{
//    foreach(EntityItem *eItem, SelectedItems().entities)
    closePersistentEditor(p);
}
*/

void SceneTreeWidget::NewEntity()
{
    if (scene.expired())
        return;

    // Create and execute dialog
    AddEntityDialog newEntDialog(framework->Ui()->MainWindow(), Qt::Tool);
    newEntDialog.resize(300, 130);
    newEntDialog.activateWindow();
    int ret = newEntDialog.exec();
    if (ret == QDialog::Rejected)
        return;

    // Process the results
    QString name = newEntDialog.EntityName().trimmed();
    bool replicated = newEntDialog.IsReplicated();
    bool temporary = newEntDialog.IsTemporary();
    AttributeChange::Type changeType = replicated ? AttributeChange::Replicate : AttributeChange::LocalOnly;

    // Create entity.
    EntityPtr entity = scene.lock()->CreateEntity(0, QStringList(), changeType, replicated);
    assert(entity);

    // Set additional properties.
    if (!name.isEmpty())
        entity->SetName(name);
    if (temporary)
        entity->SetTemporary(true);
}

void SceneTreeWidget::NewComponent()
{
    SceneTreeWidgetSelection sel = SelectedItems();
    if (sel.IsEmpty())
        return;

    AddComponentDialog *dialog = new AddComponentDialog(framework, sel.EntityIds(), framework->Ui()->MainWindow(), Qt::Tool);
    dialog->SetComponentList(framework->Scene()->ComponentTypes());
    connect(dialog, SIGNAL(finished(int)), SLOT(ComponentDialogFinished(int)));
    dialog->show();
    dialog->activateWindow();
}

void SceneTreeWidget::ComponentDialogFinished(int result)
{
    AddComponentDialog *dialog = qobject_cast<AddComponentDialog *>(sender());
    if (!dialog)
        return;

    if (result != QDialog::Accepted)
        return;

    if (scene.expired())
    {
       LogWarning("Fail to add new component to entity, since default world scene was null");
        return;
    }

    QList<entity_id_t> entities = dialog->EntityIds();
    for(int i = 0; i < entities.size(); i++)
    {
        EntityPtr entity = scene.lock()->GetEntity(entities[i]);
        if (!entity)
        {
            LogWarning("Fail to add new component to entity, since couldn't find a entity with ID:" + ::ToString<entity_id_t>(entities[i]));
            continue;
        }

        // Check if component has been already added to a entity.
        ComponentPtr comp = entity->GetComponent(dialog->TypeName(), dialog->Name());
        if (comp)
        {
            LogWarning("Fail to add a new component, cause there was already a component with a same name and a type");
            continue;
        }

        comp = framework->Scene()->CreateComponentByName(scene.lock().get(), dialog->TypeName(), dialog->Name());
        assert(comp);
        if (comp)
        {
            comp->SetReplicated(dialog->IsReplicated());
            comp->SetTemporary(dialog->IsTemporary());
            entity->AddComponent(comp, AttributeChange::Default);
        }
    }
}

void SceneTreeWidget::Delete()
{
    if (scene.expired())
        return;

    SceneTreeWidgetSelection sel = SelectedItems();
    // If we have components selected, remove them first.
    if (sel.HasComponents())
        foreach(ComponentItem *cItem, sel.components)
        {
            EntityPtr entity = cItem->Parent()->Entity();
            ComponentPtr component = cItem->Component();
            if (entity && component)
                entity->RemoveComponent(component, AttributeChange::Default);
        }

    // Remove entities.
    if (sel.HasEntities())
        foreach(entity_id_t id, SelectedItems().EntityIds())
            scene.lock()->RemoveEntity(id, AttributeChange::Replicate);
}

void SceneTreeWidget::Copy()
{
    QString sceneXml = SelectionAsXml();
    if (!sceneXml.isEmpty())
        QApplication::clipboard()->setText(sceneXml);
}

void SceneTreeWidget::Paste()
{
    if (scene.expired())
        return;

    ScenePtr scenePtr = scene.lock();
    QString errorMsg;
    QDomDocument sceneDoc("Scene");
    if (!sceneDoc.setContent(QApplication::clipboard()->text(), false, &errorMsg))
    {
        LogError("Parsing scene XML from clipboard failed: " + errorMsg);
        return;
    }

    ///\todo Move all code below, except scene->CreateContentFromXml(), to Scene.
    QDomElement sceneElem = sceneDoc.firstChildElement("scene");
    if (sceneElem.isNull())
        return;

    QDomElement entityElem = sceneElem.firstChildElement("entity");
    if (entityElem.isNull())
    {
        // No entity element, we probably have just components then. Search for component element.
        QDomElement componentElem = sceneElem.firstChildElement("component");
        if (componentElem.isNull())
        {
            LogError("SceneTreeWidget::Paste: no <entity> nor <component> element found from from XML data.");
            return;
        }

        // Get currently selected entities and paste components to them.
        foreach(entity_id_t entityId, SelectedItems().EntityIds())
        {
            EntityPtr entity = scenePtr->GetEntity(entityId);
            if (entity)
            {
                while(!componentElem.isNull())
                {
                    QString type = componentElem.attribute("type");
                    QString name = componentElem.attribute("name");
                    if (!type.isNull())
                    {
                        // If we already have component with the same type name and name, add suffix to the new component's name.
                        if (entity->GetComponent(type, name))
                            name.append("_copy");

                        ComponentPtr component = framework->Scene()->CreateComponentByName(scenePtr.get(), type, name);
                        if (component)
                        {
                            entity->AddComponent(component);
                            component->DeserializeFrom(componentElem, AttributeChange::Default);
                        }
                    }

                    componentElem = componentElem.nextSiblingElement("component");
                }

                // Rewind back to start if we are pasting components to multiple entities.
                componentElem = sceneElem.firstChildElement("component");
            }
        }
    }

    // Check sceneDoc for temporary entities and create them here.
    std::vector<EntityWeakPtr> entities;
    while(!entityElem.isNull())
    {
        QString temporaryStr = entityElem.attribute("temporary");
        bool temporary = false;
        if (!temporaryStr.isEmpty())
            temporary = ParseBool(temporaryStr);

        if (temporary)
        {
            QString replicatedStr = entityElem.attribute("sync");
            bool replicated = true;
            if (!replicatedStr.isEmpty())
                replicated = ParseBool(replicatedStr);

            entity_id_t id = replicated ? scene.lock()->NextFreeId() : scene.lock()->NextFreeIdLocal();

            if (scene.lock()->HasEntity(id)) // If the entity we are about to add conflicts in ID with an existing entity in the scene, delete the old entity.
            {
                LogDebug("SceneTreeWidget::Paste: Destroying previous entity with id " + QString::number(id) + " to avoid conflict with new created entity with the same id.");
                LogError("Warning: Invoking buggy behavior: Object with id " + QString::number(id) +" might not replicate properly!");
                scene.lock()->RemoveEntity(id, AttributeChange::Replicate); ///<@todo Consider do we want to always use Replicate
            }

            EntityPtr entity = scene.lock()->CreateEntity(id);
            if (entity)
            {
                QDomElement compElem = entityElem.firstChildElement("component");
                entity->SetTemporary(true);

                while(!compElem.isNull())
                {
                    QString type_name = compElem.attribute("type");
                    QString name = compElem.attribute("name");
                    QString compReplicatedStr = compElem.attribute("sync");
                    bool compReplicated = true;
                    if (!compReplicatedStr.isEmpty())
                        compReplicated = ParseBool(compReplicatedStr);

                    ComponentPtr new_comp = entity->GetOrCreateComponent(type_name, name, AttributeChange::Default, compReplicated);
                    if (new_comp)
                    {
                        // Trigger no signal yet when scene is in incoherent state
                        new_comp->DeserializeFrom(compElem, AttributeChange::Disconnected);
                    }

                    compElem = compElem.nextSiblingElement("component");
                }
                entities.push_back(entity);
            }

            QDomElement nextElem = entityElem.nextSiblingElement("entity");
            // Remove the temporary entities from sceneDoc.
            sceneElem.removeChild(entityElem);
            entityElem = nextElem;
        }
        else
            entityElem = entityElem.nextSiblingElement("entity");

        for(unsigned i = 0; i < entities.size(); ++i)
        {
            if (!entities[i].expired())
                scene.lock()->EmitEntityCreated(entities[i].lock().get(), AttributeChange::Default);
            if (!entities[i].expired())
            {
                EntityPtr entityShared = entities[i].lock();
                const Entity::ComponentMap &components = entityShared->Components();
                for (Entity::ComponentMap::const_iterator i = components.begin(); i != components.end(); ++i)
                    i->second->ComponentChanged(AttributeChange::Default);
            }
        }
    }

    // Create content that is not temporary
    scene.lock()->CreateContentFromXml(sceneDoc, false, AttributeChange::Replicate);
}

void SceneTreeWidget::SaveAs()
{
    if (fileDialog)
        fileDialog->close();
    fileDialog = QtUtils::SaveFileDialogNonModal(cTundraXmlFileFilter + ";;" + cTundraBinaryFileFilter,
        tr("Save SceneTreeWidgetSelection"), "", 0, this, SLOT(SaveSelectionDialogClosed(int)));
}

void SceneTreeWidget::SaveSceneAs()
{
    if (fileDialog)
        fileDialog->close();
    fileDialog = QtUtils::SaveFileDialogNonModal(cTundraXmlFileFilter + ";;" + cTundraBinaryFileFilter,
        tr("Save Scene"), "", 0, this, SLOT(SaveSceneDialogClosed(int)));
}

void SceneTreeWidget::ExportAll()
{
    if (fileDialog)
        fileDialog->close();

    if (SelectedItems().HasEntities())
    {
        // Save only selected entities
        fileDialog = QtUtils::SaveFileDialogNonModal(cTundraXmlFileFilter + ";;" + cTundraBinaryFileFilter,
            tr("Export scene"), "", 0, this, SLOT(SaveSelectionDialogClosed(int)));
    }
    else
    {
        // Save all entities in the scene
        fileDialog = QtUtils::SaveFileDialogNonModal(cTundraXmlFileFilter + ";;" + cTundraBinaryFileFilter,
            tr("Export scene"), "", 0, this, SLOT(SaveSceneDialogClosed(int)));
    }

    // Finally export assets
    connect(fileDialog, SIGNAL(finished(int)), this, SLOT(ExportAllDialogClosed(int)));
}

void SceneTreeWidget::Import()
{
    if (fileDialog)
        fileDialog->close();
    fileDialog = QtUtils::OpenFileDialogNonModal(cAllSupportedTypesFileFilter + ";;" +
        cOgreSceneFileFilter + ";;"  + cOgreMeshFileFilter + ";;" + 
#ifdef ASSIMP_ENABLED
        cMeshFileFilter + ";;" + 
#endif
        cTundraXmlFileFilter + ";;" + cTundraBinaryFileFilter + ";;" + cAllTypesFileFilter,
        tr("Import"), "", 0, this, SLOT(OpenFileDialogClosed(int)));
}

void SceneTreeWidget::OpenNewScene()
{
    if (fileDialog)
        fileDialog->close();
    fileDialog = QtUtils::OpenFileDialogNonModal(cAllSupportedTypesFileFilter + ";;" +
        cOgreSceneFileFilter + ";;" + cTundraXmlFileFilter + ";;" + cTundraBinaryFileFilter + ";;" +
        cAllTypesFileFilter, tr("Open New Scene"), "", 0, this, SLOT(OpenFileDialogClosed(int)));
}

void SceneTreeWidget::OpenEntityActionDialog()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    assert(action);
    if (!action)
        return;

    SceneTreeWidgetSelection sel = SelectedItems();
    if (sel.IsEmpty())
        return;

    if (scene.expired())
        return;

    QList<EntityWeakPtr> entities;
    foreach(entity_id_t id, sel.EntityIds())
    {
        EntityPtr e = scene.lock()->GetEntity(id);
        if (e)
            entities.append(e);
    }

    EntityActionDialog *d = new EntityActionDialog(entities, this);
    connect(d, SIGNAL(finished(int)), this, SLOT(EntityActionDialogFinished(int)));
    d->show();
}

void SceneTreeWidget::EntityActionDialogFinished(int result)
{
    EntityActionDialog *dialog = qobject_cast<EntityActionDialog *>(sender());
    if (!dialog)
        return;

    if (result == QDialog::Rejected)
        return;

    EntityAction::ExecTypeField execTypes = dialog->ExecutionType();
    QString action = dialog->Action();
    QStringList params = dialog->Parameters();

    foreach(const EntityWeakPtr &e, dialog->Entities())
        if (!e.expired())
            e.lock()->Exec(execTypes, action, params);

    // Save invoke item
    InvokeItem ii;
    ii.type = InvokeItem::Action;
    ii.objectName = QString(Entity::staticMetaObject.className());
    ii.name = action;
    ii.execTypes = execTypes;
    InvokeItem *mruItem = FindMruItem();
    ii.mruOrder = mruItem ? mruItem->mruOrder + 1 : 0;

    for(int i = 0; i < params.size(); ++i)
        ii.parameters.push_back(QVariant(params[i]));

    // Do not save duplicates and make sure that history size stays withing max size.
    QList<InvokeItem>::const_iterator it = qFind(invokeHistory, ii);
    if (it == invokeHistory.end())
    {
        while(invokeHistory.size() > historyMaxItemCount - 1)
            invokeHistory.pop_back();

        invokeHistory.push_front(ii);
    }
}

void SceneTreeWidget::OpenFunctionDialog()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    assert(action);
    if (!action)
        return;

    SceneTreeWidgetSelection sel = SelectedItems();
    if (sel.IsEmpty())
        return;

    QObjectWeakPtrList objs;
    if (sel.HasEntities())
        foreach(EntityItem *eItem, sel.entities)
        {
            EntityPtr e = eItem->Entity();
            if (e)
                objs.append(boost::dynamic_pointer_cast<QObject>(e));
        }
    else if(sel.HasComponents())
        foreach(ComponentItem *cItem, sel.components)
        {
            ComponentPtr c = cItem->Component();
            if (c)
                objs.append(boost::dynamic_pointer_cast<QObject>(c));
        }

    FunctionDialog *d = new FunctionDialog(objs, this);
    connect(d, SIGNAL(finished(int)), SLOT(FunctionDialogFinished(int)));
    d->show();
    //d->move(300,300);
}

void SceneTreeWidget::FunctionDialogFinished(int result)
{
    FunctionDialog *dialog = qobject_cast<FunctionDialog *>(sender());
    if (!dialog)
        return;

    if (result == QDialog::Rejected)
        return;

    // Get the list of parameters we will pass to the function we are invoking,
    // and update the latest values to them from the editor widgets the user inputted.
    QVariantList params;
    foreach(IArgumentType *arg, dialog->Arguments())
    {
        arg->UpdateValueFromEditor();
        params << arg->ToQVariant();
    }

    // Clear old return value from the dialog.
    dialog->SetReturnValueText("");

    foreach(const QObjectWeakPtr &o, dialog->Objects())
        if (!o.expired())
        {
            QObject *obj = o.lock().get();

            QString objName = obj->metaObject()->className();
            QString objNameWithId = objName;
            {
                Entity *e = dynamic_cast<Entity *>(obj);
                IComponent *c = dynamic_cast<IComponent *>(obj);
                if (e)
                    objNameWithId.append('(' + QString::number((uint)e->Id()) + ')');
                else if (c && !c->Name().trimmed().isEmpty())
                    objNameWithId.append('(' + c->Name() + ')');
            }

            // Invoke function.
            QString errorMsg;
            QVariant ret;
            FunctionInvoker::Invoke(obj, dialog->Function(), params, &ret, &errorMsg);

            QString retValStr;
            ///\todo For some reason QVariant::toString() cannot convert QStringList to QString properly.
            /// Convert it manually here.
            if (ret.type() == QVariant::StringList)
                foreach(QString s, ret.toStringList())
                    retValStr.append("\n" + s);
            else
                retValStr = ret.toString();

            if (errorMsg.isEmpty())
                dialog->AppendReturnValueText(objNameWithId + " " + retValStr);
            else
                dialog->AppendReturnValueText(objNameWithId + " " + errorMsg);

            // Save invoke item
            InvokeItem ii;
            ii.type = InvokeItem::Function;
            ii.parameters = params;
            ii.name = dialog->Function();
            ii.returnType = (ret.type() == QVariant::Invalid) ? QString("void") : QString(ret.typeName());
            ii.objectName = objName;
            InvokeItem *mruItem = FindMruItem();
            ii.mruOrder = mruItem ? mruItem->mruOrder + 1 : 0;

            // Do not save duplicates and make sure that history size stays withing max size.
            QList<InvokeItem>::const_iterator it = qFind(invokeHistory, ii);
            if (it == invokeHistory.end())
            {
                while(invokeHistory.size() > historyMaxItemCount - 1)
                    invokeHistory.pop_back();

                invokeHistory.push_front(ii);
            }
        }
}

void SceneTreeWidget::SaveSelectionDialogClosed(int result)
{
    QFileDialog *dialog = dynamic_cast<QFileDialog *>(sender());
    assert(dialog);
    if (!dialog)
        return;

    if (result != QDialog::Accepted)
        return;

    QStringList files = dialog->selectedFiles();
    if (files.size() != 1)
        return;

    // Check out file extension. If filename has none, use the selected name filter from the save file dialog.
    QString fileExtension;
    if (files[0].lastIndexOf('.') != -1)
    {
        fileExtension = files[0].mid(files[0].lastIndexOf('.'));
    }
    else if (dialog->selectedNameFilter() == cTundraXmlFileFilter)
    {
        fileExtension = cTundraXmlFileExtension;
        files[0].append(fileExtension);
    }
    else if (dialog->selectedNameFilter() == cTundraBinaryFileFilter)
    {
        fileExtension = cTundraBinFileExtension;
        files[0].append(fileExtension);
    }

    QFile file(files[0]);
    if (!file.open(QIODevice::WriteOnly))
    {
        LogError("Could not open file " + files[0] + " for writing.");
        return;
    }

    QByteArray bytes;

    if (fileExtension == cTundraXmlFileExtension)
    {
        bytes = SelectionAsXml().toAscii();
    }
    else // Handle all other as binary.
    {
        SceneTreeWidgetSelection sel = SelectedItems();
        if (!sel.IsEmpty())
        {
            // Assume 4MB max for now
            bytes.resize(4 * 1024 * 1024);
            kNet::DataSerializer dest(bytes.data(), bytes.size());

            dest.Add<u32>(sel.entities.size());

            foreach(EntityItem *eItem, sel.entities)
            {
                EntityPtr entity = eItem->Entity();
                assert(entity);
                if (entity)
                    entity->SerializeToBinary(dest);
            }

            bytes.resize(dest.BytesFilled());
        }
    }

    file.write(bytes);
    file.close();
}

void SceneTreeWidget::SaveSceneDialogClosed(int result)
{
    QFileDialog *dialog = dynamic_cast<QFileDialog *>(sender());
    assert(dialog);
    if (!dialog)
        return;

    if (result != QDialog::Accepted)
        return;

    QStringList files = dialog->selectedFiles();
    if (files.size() != 1)
        return;

    if (scene.expired())
        return;

    // Check out file extension. If filename has none, use the selected name filter from the save file dialog.
    bool binary = false;
    QString fileExtension;
    if (files[0].lastIndexOf('.') != -1)
    {
        fileExtension = files[0].mid(files[0].lastIndexOf('.'));
        if (fileExtension.compare(cTundraXmlFileExtension, Qt::CaseInsensitive) != 0)
            binary = true;
    }
    else if (dialog->selectedNameFilter() == cTundraXmlFileFilter)
    {
        fileExtension = cTundraXmlFileExtension;
        files[0].append(fileExtension);
    }
    else if (dialog->selectedNameFilter() == cTundraBinaryFileFilter)
    {
        fileExtension = cTundraBinFileExtension;
        files[0].append(fileExtension);
        binary = true;
    }

    if (binary)
        scene.lock()->SaveSceneBinary(files[0], false, true); /**< @todo Possibility to choose whether or not save local and temporay */
    else
        scene.lock()->SaveSceneXML(files[0], false, true); /**< @todo Possibility to choose whether or not save local and temporay */
}

void SceneTreeWidget::ExportAllDialogClosed(int result)
{
    QFileDialog *dialog = dynamic_cast<QFileDialog *>(sender());
    assert(dialog);

    if (!dialog || result != QDialog::Accepted || dialog->selectedFiles().size() != 1 || scene.expired())
        return;

    // separate path from filename
    QFileInfo fi(dialog->selectedFiles()[0]);
    QDir directory = fi.absoluteDir();
    if (!directory.exists())
        return;

    QSet<QString> assets;
    SceneTreeWidgetSelection sel = SelectedItems();
    if (!sel.HasEntities())
    {
        // Export all assets
        for(int i = 0; i < topLevelItemCount(); ++i)
        {
            EntityItem *eItem = dynamic_cast<EntityItem *>(topLevelItem(i));
            if (eItem)
                assets.unite(GetAssetRefs(eItem));
        }
    }
    else
    {
        // Export assets for selected entities
        foreach(EntityItem *eItem, sel.entities)
            assets.unite(GetAssetRefs(eItem));
    }

    savedAssets.clear();
    fetchReferences = true;
    /// \todo This is in theory a better way to get all assets in a sceene, but not all assets are currently available with this method
    ///       Once all assets are properly shown in this widget, it would be better to do it this way -cm
/*
    QTreeWidgetItemIterator it(this);
    while(*it)
    {
        AssetItem *aItem = dynamic_cast<AssetItem*>((*it));
        if (aItem)
        {
            assets.insert(aItem->id);
        }
        ++it;
    }
*/

    foreach(const QString &assetid, assets)
    {
        AssetTransferPtr transfer = framework->Asset()->RequestAsset(assetid);

        QString filename = directory.absolutePath();
        QString assetName = AssetAPI::ExtractFilenameFromAssetRef(assetid);
        filename += "/" + assetName;

        fileSaves.insert(transfer->source.ref, filename);
        connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), this, SLOT(AssetLoaded(AssetPtr)));
    }
}

QSet<QString> SceneTreeWidget::GetAssetRefs(const EntityItem *eItem) const
{
    assert(scene.lock());
    QSet<QString> assets;

    EntityPtr entity = eItem->Entity();
    if (entity)
    {
        int entityChildCount = eItem->childCount();
        for(int j = 0; j < entityChildCount; ++j)
        {
            ComponentItem *cItem = dynamic_cast<ComponentItem *>(eItem->child(j));
            if (!cItem)
                continue;
            ComponentPtr comp = cItem->Component();
            if (!comp)
                continue;

            const Entity::ComponentMap &components = entity->Components();
            for (Entity::ComponentMap::const_iterator i = components.begin(); i != components.end(); ++i)
                foreach(IAttribute *attr, i->second->Attributes())
                {
                    if (!attr)
                        continue;
                    
                    if (attr->TypeName() == "assetreference")
                    {
                        Attribute<AssetReference> *assetRef = dynamic_cast<Attribute<AssetReference> *>(attr);
                        if (assetRef)
                            assets.insert(assetRef->Get().ref);
                    }
                    else if (attr->TypeName() == "assetreferencelist")
                    {
                        Attribute<AssetReferenceList> *assetRefs = dynamic_cast<Attribute<AssetReferenceList> *>(attr);
                        if (assetRefs)
                            for(int i = 0; i < assetRefs->Get().Size(); ++i)
                                assets.insert(assetRefs->Get()[i].ref);
                    }
                }
        }
    }

    return assets;
}

void SceneTreeWidget::OpenFileDialogClosed(int result)
{
    QFileDialog *dialog = dynamic_cast<QFileDialog *>(sender());
    assert(dialog);
    if (!dialog)
        return;

    if (result != QDialog::Accepted)
        return;

    foreach(QString filename, dialog->selectedFiles())
    {
        bool clearScene = false;
        ///\todo This is awful hack, find better way
        if (dialog->windowTitle() == tr("Open New Scene"))
            clearScene = true;

        SceneStructureModule *sceneStruct = framework->GetModule<SceneStructureModule>();
        if (sceneStruct)
            sceneStruct->InstantiateContent(filename, float3::zero, clearScene);
        else
            LogError("Could not retrieve SceneStructureModule. Cannot instantiate content.");
    }
}

void SceneTreeWidget::InvokeActionTriggered()
{
    QAction *action = dynamic_cast<QAction *>(sender());
    assert(action);
    if (!action)
        return;

    SceneTreeWidgetSelection sel = SelectedItems();
    if (sel.IsEmpty())
        return;

    InvokeItem *invokedItem = 0;
    for(int i = 0; i< invokeHistory.size(); ++i)
        if (invokeHistory[i].ToString() == action->text())
        {
            invokedItem = &invokeHistory[i];
            break;
        }

    InvokeItem *mruItem = FindMruItem();
    assert(mruItem);
    assert(invokedItem);
    invokedItem->mruOrder = mruItem->mruOrder + 1;

    // Gather target objects.
    QList<EntityWeakPtr> entities;
    QObjectList objects;
    QObjectWeakPtrList objectPtrs;

    foreach(EntityItem *eItem, sel.entities)
        if (eItem->Entity())
        {
            entities << eItem->Entity();
            objects << eItem->Entity().get();
            objectPtrs << boost::dynamic_pointer_cast<QObject>(eItem->Entity());
        }

    foreach(ComponentItem *cItem, sel.components)
        if (cItem->Component())
        {
            objects << cItem->Component().get();
            objectPtrs << boost::dynamic_pointer_cast<QObject>(cItem->Component());
        }

    // Shift+click opens existing invoke history item editable in dialog.
    bool openForEditing = contextMenu && contextMenu->shiftDown;
    if (invokedItem->type == InvokeItem::Action)
    {
        if (openForEditing)
        {
            EntityActionDialog *d = new EntityActionDialog(entities, *invokedItem, this);
            connect(d, SIGNAL(finished(int)), this, SLOT(EntityActionDialogFinished(int)));
            d->show();
        }
        else
        {
            foreach(const EntityWeakPtr &e, entities)
                e.lock()->Exec(invokedItem->execTypes, invokedItem->name, invokedItem->parameters);
            qSort(invokeHistory.begin(), invokeHistory.end(), qGreater<InvokeItem>());
        }
    }
    else if (invokedItem->type == InvokeItem::Function)
    {
        if (openForEditing)
        {
            FunctionDialog *d = new FunctionDialog(objectPtrs, *invokedItem, this);
            connect(d, SIGNAL(finished(int)), SLOT(FunctionDialogFinished(int)));
            d->show();
            d->move(300,300);
        }
        else
        {
            foreach(QObject *obj, objects)
            {
                QVariant retVal;
                FunctionInvoker::Invoke(obj, invokedItem->name, invokedItem->parameters, &retVal);
                LogInfo("Invoked function returned " + retVal.toString());
            }
            qSort(invokeHistory.begin(), invokeHistory.end(), qGreater<InvokeItem>());
        }
    }
}

void SceneTreeWidget::SaveAssetAs()
{
    SceneTreeWidgetSelection sel = SelectedItems();
    QString assetName;

    if (fileDialog)
        fileDialog->close();

    if (sel.assets.size() == 1)
    {
        assetName = AssetAPI::ExtractFilenameFromAssetRef(sel.assets[0]->id);
        fileDialog = QtUtils::SaveFileDialogNonModal("", tr("Save Asset As"), assetName, 0, this, SLOT(SaveAssetDialogClosed(int)));
    }
    else
    {
        QtUtils::DirectoryDialogNonModal(tr("Select Directory"), "", 0, this, SLOT(SaveAssetDialogClosed(int)));
    }
}

void SceneTreeWidget::SaveAssetDialogClosed(int result)
{
    QFileDialog *dialog = dynamic_cast<QFileDialog *>(sender());
    assert(dialog);

    if (!dialog || result != QDialog::Accepted || dialog->selectedFiles().isEmpty() || scene.expired())
        return;

    QStringList files = dialog->selectedFiles();
    SceneTreeWidgetSelection sel = SelectedItems();
    
    bool isDir = QDir(files[0]).exists();

    if ((sel.assets.size() == 1 && isDir) || (sel.assets.size() > 1 && !isDir))
    {
        // should not happen normally, so just log error. No prompt for user.
        LogError("Could not save asset: no such directory.");
        return;
    }

    savedAssets.clear();
    fetchReferences = false;
    foreach(AssetRefItem *aItem, sel.assets)
    {
        AssetTransferPtr transfer = framework->Asset()->RequestAsset(aItem->id);

        // if saving multiple assets, append filename to directory
        QString filename = files[0];
        if (isDir)
        {
            QString assetName = AssetAPI::ExtractFilenameFromAssetRef(aItem->id);
            filename += "/" + assetName;
        }

        fileSaves.insert(transfer->source.ref, filename);
        connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), this, SLOT(AssetLoaded(AssetPtr)));
    }
}

void SceneTreeWidget::AssetLoaded(AssetPtr asset)
{
    assert(asset.get());
    if (!asset)
    {
        LogError("Null asset pointer.");
        return;
    }

    QString filename = fileSaves.take(asset->Name());
    if (!savedAssets.contains(filename))
    {
        savedAssets.insert(filename);

        QString param;
        if (asset->Type().contains("texture", Qt::CaseInsensitive))
            param = filename.right(filename.size() - filename.lastIndexOf('.') - 1);
        if (!asset->SaveToFile(filename, param))
        {
            LogError("Could not save asset to file " + filename + ".");
            QMessageBox box(QMessageBox::Warning, tr("Save asset"), tr("Failed to save asset."), QMessageBox::Ok);
            box.setInformativeText(tr("Please check the selected storage device can be written to."));
            box.exec();
        }

        if (fetchReferences)
            foreach(AssetReference ref, asset->FindReferences())
                if (!savedAssets.contains(ref.ref))
                {
                    AssetTransferPtr transfer = framework->Asset()->RequestAsset(ref.ref);
                    connect(transfer.get(), SIGNAL(Succeeded(AssetPtr)), this, SLOT(AssetLoaded(AssetPtr)));

                    QString oldAssetName = AssetAPI::ExtractFilenameFromAssetRef(filename);
                    QString newAssetName = AssetAPI::ExtractFilenameFromAssetRef(ref.ref);
                    filename.replace(oldAssetName, newAssetName);
                    fileSaves.insert(transfer->source.ref, filename);
                }
    }
}

void SceneTreeWidget::HandleECEditorDestroyed(QObject *obj)
{
    QList<QPointer<ECEditorWindow> >::iterator iter = ecEditors.begin();
    while(iter != ecEditors.end())
    {
        if (*iter == obj)
        {
            ecEditors.erase(iter);
            break;
        }
        ++iter;
    }
}

void SceneTreeWidget::ConvertEntityToLocal()
{
    ScenePtr scn = scene.lock();
    if (scn)
        foreach(EntityItem *item, SelectedItems().entities)
        {
            EntityPtr orgEntity = item->Entity();
            if (orgEntity && !orgEntity->IsLocal())
            {
                EntityPtr newEntity = orgEntity->Clone(true, orgEntity->IsTemporary());
                if (newEntity)
                    scn->RemoveEntity(orgEntity->Id()); // Creation successful, remove the original.
            }
        }
}

void SceneTreeWidget::ConvertEntityToReplicated()
{
    ScenePtr scn = scene.lock();
    if (scn)
        foreach(EntityItem *item, SelectedItems().entities)
        {
            EntityPtr orgEntity = item->Entity();
            if (orgEntity && orgEntity->IsLocal())
            {
                EntityPtr newEntity = orgEntity->Clone(false, orgEntity->IsTemporary());
                if (newEntity)
                    scn->RemoveEntity(orgEntity->Id()); // Creation successful, remove the original.
            }
        }
}

void SceneTreeWidget::SetAsTemporary(bool temporary)
{
    foreach(EntityItem *item, SelectedItems().entities)
        if (item->Entity())
        {
            item->Entity()->SetTemporary(temporary);
            item->SetText(item->Entity().get());
        }
}

void SceneTreeWidget::DeleteFromSource()
{
    // AssetAPI::DeleteAssetFromStorage() signals will start deletion of tree widget asset items:
    // Gather the asset refs to a separate list beforehand in order to prevent crash.
    QStringList assetRefs, assetsToBeDeleted;
    foreach(AssetItem *item, SelectedItems().assets)
        if (item->Asset())
        {
            assetRefs << item->Asset()->Name();
            assetsToBeDeleted << item->Asset()->DiskSource();
        }

    QMessageBox msgBox(QMessageBox::Warning, tr("Delete From Source"),
        tr("Are you sure want to delete the selected asset(s) permanently from the source?\n"),
        QMessageBox::Ok | QMessageBox::Cancel, this);
    msgBox.setDetailedText(assetsToBeDeleted.join("\n"));
    int ret = msgBox.exec();
    if (ret == QMessageBox::Ok)
        foreach(QString ref, assetRefs)
            framework->Asset()->DeleteAssetFromStorage(ref);
}

void SceneTreeWidget::DeleteFromCache()
{
    if (!framework->Asset()->GetAssetCache())
    {
        LogError("Cannot delete asset from cache: Not running Tundra with an asset cache!");
        return;
    }
    foreach(AssetItem *item, SelectedItems().assets)
        if (item->Asset())
            framework->Asset()->GetAssetCache()->DeleteAsset(item->Asset()->Name());
}

void SceneTreeWidget::Forget()
{
    foreach(AssetItem *item, SelectedItems().assets)
        if (item->Asset())
            framework->Asset()->ForgetAsset(item->Asset(), false);
}

void SceneTreeWidget::ReloadFromSource()
{
    foreach(AssetItem *item, SelectedItems().assets)
        if (item->Asset())
        {
            QString assetRef = item->Asset()->Name();
            // Make a 'forced request' of the existing asset. This will cause a full re-download of the asset
            // and the newly downloaded data will be deserialized to the existing asset object.
            framework->Asset()->RequestAsset(assetRef, item->Asset()->Type(), true);
        }
}

void SceneTreeWidget::ReloadFromCache()
{
    foreach(AssetItem *item, SelectedItems().assets)
        if (item->Asset())
            item->Asset()->LoadFromCache();
}

void SceneTreeWidget::Unload()
{
    foreach(AssetItem *item, SelectedItems().assets)
        if (item->Asset())
            item->Asset()->Unload();
}

void SceneTreeWidget::OpenFileLocation()
{
    QList<AssetItem *> selection = SelectedItems().assets;
    if (selection.isEmpty() || selection.size() < 1)
        return;

    AssetItem *item = selection.first();
    if (item->Asset() && !item->Asset()->DiskSource().isEmpty())
    {
        bool success = false;
#ifdef _WINDOWS
        // Custom code for Windows, as we want to use explorer.exe with the /select switch.
        // Craft command line string, use the full filename, not directory.
        QString path = QDir::toNativeSeparators(item->Asset()->DiskSource());
        WCHAR commandLineStr[256] = {};
        WCHAR wcharPath[256] = {};
        mbstowcs(wcharPath, path.toStdString().c_str(), 254);
        wsprintf(commandLineStr, L"explorer.exe /select,%s", wcharPath);

        STARTUPINFO startupInfo;
        memset(&startupInfo, 0, sizeof(STARTUPINFO));
        startupInfo.cb = sizeof(STARTUPINFO);
        PROCESS_INFORMATION processInfo;
        memset(&processInfo, 0, sizeof(PROCESS_INFORMATION));
        success = CreateProcessW(NULL, commandLineStr, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS,
            NULL, NULL, &startupInfo, &processInfo);

        CloseHandle(processInfo.hProcess);
        CloseHandle(processInfo.hThread);
#else
        QString path = QDir::toNativeSeparators(QFileInfo(item->Asset()->DiskSource()).dir().path());
        success = QDesktopServices::openUrl(QUrl("file:///" + path, QUrl::TolerantMode));
#endif
        if (!success)
            LogError("AssetTreeWidget::OpenFileLocation: failed to open " + path);
    }
}

void SceneTreeWidget::OpenInExternalEditor()
{
    QList<AssetItem *> selection = SelectedItems().assets;
    if (selection.isEmpty() || selection.size() < 1)
        return;

    AssetItem *item = selection.first();
    if (item->Asset() && !item->Asset()->DiskSource().isEmpty())
        if (!QDesktopServices::openUrl(QUrl("file:///" + item->Asset()->DiskSource(), QUrl::TolerantMode)))
            LogError("AssetTreeWidget::OpenInExternalEditor: failed to open " + item->Asset()->DiskSource());
}

void SceneTreeWidget::CopyAssetRef()
{
    QList<AssetItem *> sel = SelectedItems().assets;
    if (sel.size() == 1 && sel.first()->Asset())
        QApplication::clipboard()->setText(sel.first()->Asset()->Name());
}

void SceneTreeWidget::Clone()
{
    QList<AssetItem *> sel = SelectedItems().assets;
    if (sel.isEmpty())
        return;

    CloneAssetDialog *dialog = new CloneAssetDialog(sel.first()->Asset(), framework->Asset(), this);
    connect(dialog, SIGNAL(finished(int)), SLOT(CloneAssetDialogClosed(int)));
    dialog->show();
}

void SceneTreeWidget::Export()
{
    QList<AssetItem *> sel = SelectedItems().assets;
    if (sel.isEmpty())
        return;

    if (sel.size() == 1)
    {
        QString ref = sel.first()->Asset() ? sel.first()->Asset()->Name() : "";
        QString assetName= AssetAPI::ExtractFilenameFromAssetRef(ref);
        QtUtils::SaveFileDialogNonModal("", tr("Save Asset As"), assetName, 0, this, SLOT(SaveAssetDialogClosed(int)));
    }
    else
    {
        QtUtils::DirectoryDialogNonModal(tr("Select Directory"), "", 0, this, SLOT(SaveAssetDialogClosed(int)));
    }
}

void SceneTreeWidget::RequestNewAsset()
{
    RequestNewAssetDialog *dialog = new RequestNewAssetDialog(framework->Asset(), this);
    connect(dialog, SIGNAL(finished(int)), SLOT(RequestNewAssetDialogClosed(int)));
    dialog->show();
}

void SceneTreeWidget::CreateAsset()
{
    QAction *action = qobject_cast<QAction *>(sender());
    assert(action);
    if (!action)
        return;
    QString assetType = action->objectName();
    QString assetName = framework->Asset()->GenerateUniqueAssetName(assetType, tr("New"));
    framework->Asset()->CreateNewAsset(assetType, assetName);
}

void SceneTreeWidget::MakeDefaultStorage()
{
    AssetTreeWidgetSelection sel = SelectedItems();
    if (sel.storages.size() == 1)
    {
        framework->Asset()->SetDefaultAssetStorage(sel.storages.first()->Storage());
        //QString storageName = selected.first()->data(0, Qt::UserRole).toString();
        //framework->Asset()->SetDefaultAssetStorage(framework->Asset()->GetAssetStorageByName(storageName));
    }

    AssetsWindow *parent = dynamic_cast<AssetsWindow*>(parentWidget());
    if (parent)
        parent->PopulateTreeWidget();
}

void SceneTreeWidget::RemoveStorage()
{
    foreach(AssetStorageItem *item, SelectedItems().storages)
    {
        //QString storageName = item->data(0, Qt::UserRole).toString();
        //framework->Asset()->RemoveAssetStorage(storageName);
        if (item->Storage())
            framework->Asset()->RemoveAssetStorage(item->Storage()->Name());
    }

    AssetsWindow *parent = dynamic_cast<AssetsWindow*>(parentWidget());
    if (parent)
        parent->PopulateTreeWidget();
}