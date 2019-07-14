// Copyright (C) 2019 J�r�me Leclercq
// This file is part of the "Burgwar Map Editor" project
// For conditions of distribution and use, see copyright notice in LICENSE

#include <MapEditor/Widgets/EditorWindow.hpp>
#include <CoreLib/Scripting/ScriptingContext.hpp>
#include <MapEditor/Logic/BasicEditorMode.hpp>
#include <MapEditor/Logic/TileMapEditorMode.hpp>
#include <MapEditor/Scripting/EditorScriptingLibrary.hpp>
#include <MapEditor/Widgets/EntityInfoDialog.hpp>
#include <MapEditor/Widgets/LayerInfoDialog.hpp>
#include <MapEditor/Widgets/MapCanvas.hpp>
#include <MapEditor/Widgets/MapInfoDialog.hpp>
#include <QtCore/QStringBuilder>
#include <QtGui/QKeyEvent>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <iostream>

namespace bw
{
	EditorWindow::EditorWindow()
	{
		RegisterEditorConfig();

		if (!m_config.LoadFromFile("editorconfig.lua"))
			throw std::runtime_error("Failed to load config file");

		const std::string& editorAssetsFolder = m_config.GetStringOption("Assets.EditorFolder");
		const std::string& gameResourceFolder = m_config.GetStringOption("Assets.ResourceFolder");
		const std::string& scriptFolder = m_config.GetStringOption("Assets.ScriptFolder");

		std::shared_ptr<VirtualDirectory> virtualDir = std::make_shared<VirtualDirectory>(scriptFolder);

		m_scriptingContext = std::make_shared<ScriptingContext>(virtualDir);
		m_scriptingContext->LoadLibrary(std::make_shared<EditorScriptingLibrary>());
		m_scriptingContext->GetLuaState()["Editor"] = this;

		m_entityStore.emplace(gameResourceFolder, m_scriptingContext);

		VirtualDirectory::Entry entry;
		
		if (virtualDir->GetEntry("entities", &entry))
		{
			std::filesystem::path path = "entities";

			VirtualDirectory::VirtualDirectoryEntry& directory = std::get<VirtualDirectory::VirtualDirectoryEntry>(entry);
			directory->Foreach([&](const std::string& entryName, const VirtualDirectory::Entry& entry)
			{
				m_entityStore->LoadElement(std::holds_alternative<VirtualDirectory::VirtualDirectoryEntry>(entry), path / entryName);
			});
		}

		// Load some resources

		Nz::MaterialRef arrowMat = Nz::Material::New("Translucent2D");
		arrowMat->EnableDepthBuffer(false);
		arrowMat->SetDiffuseMap(editorAssetsFolder + "/arrow.png");

		Nz::MaterialLibrary::Register("GizmoArrow", arrowMat);

		Nz::ImageLibrary::Register("Eraser", Nz::Image::LoadFromFile(editorAssetsFolder + "/eraser.png"));

		Nz::MaterialRef selectionMaterial = Nz::Material::New("Translucent2D");
		selectionMaterial->SetDiffuseMap(editorAssetsFolder + "/tile_selection.png");

		Nz::MaterialLibrary::Register("TileSelection", selectionMaterial);

		// GUI
		BuildMenu();

		m_canvas = new MapCanvas(*this);

		m_canvas->OnCameraZoomFactorUpdated.Connect([this](MapCanvas* /*emitter*/, float factor)
		{
			statusBar()->showMessage(tr("Zoom level: %1%").arg(static_cast<int>(std::round(factor * 100.f))));
		});

		m_canvas->OnDeleteEntity.Connect([this](MapCanvas* /*emitter*/, Ndk::EntityId canvasIndex)
		{
			auto it = m_entityIndexes.find(canvasIndex);
			assert(it != m_entityIndexes.end());

			OnDeleteEntity(it.value());
		});

		m_canvas->OnEntityPositionUpdated.Connect([this](MapCanvas* /*emitter*/, Ndk::EntityId canvasIndex, const Nz::Vector2f& newPosition)
		{
			assert(m_currentLayer.has_value());

			auto it = m_entityIndexes.find(canvasIndex);
			assert(it != m_entityIndexes.end());

			std::size_t entityIndex = it.value();

			auto& layer = m_workingMap.GetLayer(m_currentLayer.value());

			auto& layerEntity = layer.entities[entityIndex];
			layerEntity.position = newPosition;
		});

		m_canvas->OnCanvasMouseButtonPressed.Connect([this](MapCanvas* /*emitter*/, const Nz::WindowEvent::MouseButtonEvent& mouseButton)
		{
			m_currentMode->OnMouseButtonPressed(mouseButton);
		});

		m_canvas->OnCanvasMouseButtonReleased.Connect([this](MapCanvas* /*emitter*/, const Nz::WindowEvent::MouseButtonEvent& mouseButton)
		{
			m_currentMode->OnMouseButtonReleased(mouseButton);
		});

		m_canvas->OnCanvasMouseEntered.Connect([this](MapCanvas* /*emitter*/)
		{
			m_currentMode->OnMouseEntered();
		});

		m_canvas->OnCanvasMouseLeft.Connect([this](MapCanvas* /*emitter*/)
		{
			m_currentMode->OnMouseLeft();
		});

		m_canvas->OnCanvasMouseMoved.Connect([this](MapCanvas* /*emitter*/, const Nz::WindowEvent::MouseMoveEvent& mouseMove)
		{
			m_currentMode->OnMouseMoved(mouseMove);
		});

		setCentralWidget(m_canvas);

		QDockWidget* layerListDock = new QDockWidget("Layer list", this);

		m_layerList = new QListWidget;
		connect(m_layerList, &QListWidget::currentRowChanged, this, &EditorWindow::OnLayerChanged);
		connect(m_layerList, &QListWidget::itemDoubleClicked, this, &EditorWindow::OnLayerDoubleClicked);

		layerListDock->setWidget(m_layerList);

		addDockWidget(Qt::RightDockWidgetArea, layerListDock);

		QDockWidget* entityListDock = new QDockWidget("Layer entities", this);

		m_entityList = new QListWidget;
		connect(m_entityList, &QListWidget::itemDoubleClicked, this, &EditorWindow::OnEntityDoubleClicked);
		connect(m_entityList, &QListWidget::itemSelectionChanged, this, &EditorWindow::OnEntitySelectionUpdate);

		entityListDock->setWidget(m_entityList);

		addDockWidget(Qt::RightDockWidgetArea, entityListDock);

		QToolBar* toolBar = new QToolBar;
		QAction* createMap = toolBar->addAction(QIcon(QPixmap((editorAssetsFolder + "/gui/icons/file-48.png").c_str())), tr("Create map..."));
		connect(createMap, &QAction::triggered, this, &EditorWindow::OnCreateMap);

		QAction* openMap = toolBar->addAction(QIcon(QPixmap((editorAssetsFolder + "/gui/icons/opened_folder-48.png").c_str())), tr("Open map..."));
		connect(openMap, &QAction::triggered, this, &EditorWindow::OnOpenMap);

		m_saveMapToolbar = toolBar->addAction(QIcon(QPixmap((editorAssetsFolder + "/gui/icons/icons8-save-48.png").c_str())), tr("Save map..."));
		connect(m_saveMapToolbar, &QAction::triggered, this, &EditorWindow::OnSaveMap);

		toolBar->addSeparator();

		m_createEntityActionToolbar = toolBar->addAction(QIcon(QPixmap((editorAssetsFolder + "/gui/icons/idea-48.png").c_str())), tr("Create entity"));
		connect(m_createEntityActionToolbar, &QAction::triggered, this, &EditorWindow::OnCreateEntity);

		QDockWidget* toolbarDock = new QDockWidget("Toolbar", this);
		toolbarDock->setWidget(toolBar);

		addDockWidget(Qt::TopDockWidgetArea, toolbarDock);

		resize(1280, 720);
		setWindowTitle(tr("Burg'war map editor"));

		ClearWorkingMap();

		m_currentMode = std::make_shared<BasicEditorMode>(*this);
		m_currentMode->OnEnter();

		statusBar()->showMessage(tr("Ready"), 0);
	}

	EditorWindow::~EditorWindow()
	{
		m_currentMode->OnLeave();
		m_currentMode.reset();

		// Delete canvas before releasing everything else
		delete m_canvas;
	}

	void EditorWindow::ClearWorkingMap()
	{
		UpdateWorkingMap(Map());
	}

	void EditorWindow::SelectEntity(Ndk::EntityId entityId)
	{
		auto it = m_entityIndexes.find(entityId);
		assert(it != m_entityIndexes.end());

		std::size_t entityIndex = it.value();

		m_entityList->clearSelection();
		m_entityList->setItemSelected(m_entityList->item(int(entityIndex)), true);
	}

	void EditorWindow::SwitchToMode(std::shared_ptr<EditorMode> editorMode)
	{
		m_currentMode->OnLeave();
		m_currentMode = std::move(editorMode);
		m_currentMode->OnEnter();
	}

	void EditorWindow::UpdateWorkingMap(Map map, std::filesystem::path mapPath)
	{
		m_workingMap = std::move(map);
		m_workingMapPath = std::move(mapPath);

		bool enableMapActions = m_workingMap.IsValid();

		m_compileMap->setEnabled(enableMapActions);
		m_createEntityActionToolbar->setEnabled(enableMapActions);
		m_saveMap->setEnabled(enableMapActions);
		m_saveMapToolbar->setEnabled(enableMapActions);

		if (m_workingMap.IsValid())
		{
			m_layerList->clear();
			for (std::size_t layerIndex = 0; layerIndex < m_workingMap.GetLayerCount(); ++layerIndex)
			{
				const auto& layer = m_workingMap.GetLayer(layerIndex);

				QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(layer.name));
				item->setData(Qt::UserRole, qulonglong(layerIndex));

				m_layerList->addItem(item);
			}
		}
	}

	bool EditorWindow::event(QEvent* e)
	{
		switch (e->type())
		{
			case QEvent::KeyPress:
			{
				QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
				if (keyEvent->key() == Qt::Key_Delete)
					OnDeleteEntity();
			}
		}

		return QMainWindow::event(e);
	}

	void EditorWindow::BuildMenu()
	{
		QMenu* fileMenu = menuBar()->addMenu(tr("&File"));
		QAction* createMap = fileMenu->addAction(tr("Create map..."));
		connect(createMap, &QAction::triggered, this, &EditorWindow::OnCreateMap);

		QAction* openMap = fileMenu->addAction(tr("Open map..."));
		connect(openMap, &QAction::triggered, this, &EditorWindow::OnOpenMap);

		m_saveMap = fileMenu->addAction(tr("Save map..."));
		connect(m_saveMap, &QAction::triggered, this, &EditorWindow::OnSaveMap);

		fileMenu->addSeparator();

		m_compileMap = fileMenu->addAction(tr("Compile map..."));
		connect(m_compileMap, &QAction::triggered, this, &EditorWindow::OnCompileMap);

		QMenu* mapMenu = menuBar()->addMenu(tr("&Map"));

		QMenu* showMenu = menuBar()->addMenu(tr("&Show"));

		QMenu* helpMenu = menuBar()->addMenu(tr("&Help"));
		QAction* aboutQt = helpMenu->addAction(tr("About Qt..."));
		aboutQt->setMenuRole(QAction::AboutQtRole);
		connect(aboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
	}

	void EditorWindow::OnCompileMap()
	{
		QString filter("*.bmap");
		QString fileName = QFileDialog::getSaveFileName(this, tr("Where to save compiled map file"), QString(), filter, &filter);
		if (fileName.isEmpty())
			return;

		if (!fileName.endsWith(".bmap"))
			fileName += ".bmap";

		if (m_workingMap.Compile(fileName.toStdString()))
			QMessageBox::information(this, tr("Compilation succeeded"), tr("Map has been successfully compiled"), QMessageBox::Ok);
		else
			QMessageBox::critical(this, tr("Failed to compile map"), tr("Map failed to compile"), QMessageBox::Ok);
	}

	void EditorWindow::OnCreateEntity()
	{
		std::size_t layerIndex = static_cast<std::size_t>(m_layerList->currentRow());

		EntityInfoDialog* createEntityDialog = new EntityInfoDialog(*m_entityStore, *m_scriptingContext, this);
		connect(createEntityDialog, &QDialog::accepted, [this, createEntityDialog, layerIndex]()
		{
			const EntityInfo& entityInfo = createEntityDialog->GetEntityInfo();

			auto& layer = m_workingMap.GetLayer(layerIndex);
			
			std::size_t entityIndex = layer.entities.size();
			auto& layerEntity = layer.entities.emplace_back();
			layerEntity.entityType = entityInfo.entityClass;
			layerEntity.name = entityInfo.entityName;
			layerEntity.position = entityInfo.position;
			layerEntity.properties = entityInfo.properties;
			layerEntity.rotation = entityInfo.rotation;

			RegisterEntity(entityIndex);
		});

		createEntityDialog->exec();
	}

	void EditorWindow::OnCreateMap()
	{
		MapInfoDialog* createMapDialog = new MapInfoDialog(this);
		connect(createMapDialog, &QDialog::accepted, [this, createMapDialog]()
		{
			MapInfo mapInfo = createMapDialog->GetMapInfo();

			UpdateWorkingMap(Map(mapInfo));
		});
		createMapDialog->exec();
	}

	void EditorWindow::OnDeleteEntity()
	{
		QList<QListWidgetItem*> items = m_entityList->selectedItems();
		if (!items.empty())
		{
			assert(items.size() == 1);

			QListWidgetItem* item = items.front();
			std::size_t entityIndex = static_cast<std::size_t>(item->data(Qt::UserRole).value<qulonglong>());

			OnDeleteEntity(entityIndex);

			m_entityList->clearSelection();
		}
	}

	void EditorWindow::OnDeleteEntity(std::size_t entityIndex)
	{
		auto& layer = m_workingMap.GetLayer(m_currentLayer.value());

		auto& layerEntity = layer.entities[entityIndex];

		QString warningText = tr("You are about to delete entity %1 of type %2, are you sure you want to do that?").arg(QString::fromStdString(layerEntity.name)).arg(QString::fromStdString(layerEntity.entityType));
		QMessageBox::StandardButton response = QMessageBox::warning(this, tr("Are you sure?"), warningText, QMessageBox::Yes | QMessageBox::Cancel);
		if (response == QMessageBox::Yes)
		{
			QListWidgetItem* item = m_entityList->takeItem(int(entityIndex));
			Ndk::EntityId canvasId = item->data(Qt::UserRole + 1).value<Ndk::EntityId>();

			delete item;

			m_canvas->DeleteEntity(canvasId);

			m_entityIndexes.erase(canvasId);

			layer.entities.erase(layer.entities.begin() + entityIndex);

			// FIXME...
			for (auto it = m_entityIndexes.begin(); it != m_entityIndexes.end(); ++it)
			{
				if (it->second >= entityIndex)
				{
					std::size_t newEntityIndex = --it.value();
					m_entityList->item(newEntityIndex)->setData(Qt::UserRole, qulonglong(newEntityIndex));
				}
			}
		}
	}

	void EditorWindow::OnEntityDoubleClicked(QListWidgetItem* item)
	{
		if (!item)
			return;

		std::size_t entityIndex = static_cast<std::size_t>(item->data(Qt::UserRole).value<qulonglong>());
		Ndk::EntityId canvasId = item->data(Qt::UserRole + 1).value<Ndk::EntityId>();
		std::size_t layerIndex = static_cast<std::size_t>(m_layerList->currentRow());

		auto& layer = m_workingMap.GetLayer(layerIndex);

		auto& layerEntity = layer.entities[entityIndex];

		EntityInfo entityInfo;
		entityInfo.entityClass = layerEntity.entityType;
		entityInfo.entityName = layerEntity.name;
		entityInfo.position = layerEntity.position;
		entityInfo.properties = layerEntity.properties;
		entityInfo.rotation = layerEntity.rotation;

		const auto& entity = m_canvas->GetWorld().GetEntity(canvasId);

		EntityInfoDialog* editEntityDialog = new EntityInfoDialog(*m_entityStore, *m_scriptingContext, entity, std::move(entityInfo), this);
		connect(editEntityDialog, &QDialog::accepted, [this, editEntityDialog, entityIndex, layerIndex, item, canvasId]()
		{
			const EntityInfo& entityInfo = editEntityDialog->GetEntityInfo();

			auto& layer = m_workingMap.GetLayer(layerIndex);

			auto& layerEntity = layer.entities[entityIndex];
			layerEntity.entityType = entityInfo.entityClass;
			layerEntity.position = entityInfo.position;
			layerEntity.properties = entityInfo.properties;
			layerEntity.rotation = entityInfo.rotation;

			//m_canvas->UpdateEntityPositionAndRotation(canvasId, layerEntity.position, layerEntity.rotation);
			m_canvas->DeleteEntity(canvasId);
			m_entityIndexes.erase(canvasId);

			Ndk::EntityId newCanvasId = m_canvas->CreateEntity(layerEntity.entityType, layerEntity.position, layerEntity.rotation, layerEntity.properties)->GetId();
			m_entityIndexes.emplace(newCanvasId, entityIndex);
			item->setData(Qt::UserRole + 1, newCanvasId);

			bool resetItemName = false;
			if (layerEntity.entityType != entityInfo.entityClass)
			{
				layerEntity.entityType = entityInfo.entityClass;
				resetItemName = true;
			}

			if (layerEntity.name != entityInfo.entityName)
			{
				layerEntity.name = entityInfo.entityName;
				resetItemName = true;
			}

			if (resetItemName)
			{
				QString entryName = QString::fromStdString(layerEntity.entityType);
				if (!layerEntity.name.empty())
					entryName = entryName % " (" % QString::fromStdString(layerEntity.name) % ")";

				item->setText(entryName);
			}

			if (item->isSelected())
			{
				item->setSelected(false);
				item->setSelected(true);
			}
		});

		editEntityDialog->exec();
	}

	void EditorWindow::OnEntitySelectionUpdate()
	{
		QList<QListWidgetItem*> items = m_entityList->selectedItems();
		if (!items.empty())
		{
			assert(items.size() == 1);

			QListWidgetItem* item = items.front();

			std::size_t entityIndex = static_cast<std::size_t>(item->data(Qt::UserRole).value<qulonglong>());
			Ndk::EntityId canvasId = item->data(Qt::UserRole + 1).value<Ndk::EntityId>();
			std::size_t layerIndex = static_cast<std::size_t>(m_layerList->currentRow());

			m_canvas->EditEntityPosition(canvasId);
		}
		else
			m_canvas->ClearEntitySelection();
	}

	void EditorWindow::OnLayerChanged(int layerIndex)
	{
		if (layerIndex == -1)
		{
			m_currentLayer.reset();
			m_entityIndexes.clear();
			return;
		}

		assert(layerIndex >= 0);
		std::size_t layerIdx = static_cast<std::size_t>(layerIndex);

		m_currentLayer = layerIndex;

		assert(layerIdx < m_workingMap.GetLayerCount());
		auto& layer = m_workingMap.GetLayer(layerIdx);

		m_entityList->clear();

		m_canvas->UpdateBackgroundColor(layer.backgroundColor);

		m_canvas->ClearEntities();
		m_entityIndexes.clear();

		for (std::size_t entityIndex = 0; entityIndex < layer.entities.size(); ++entityIndex)
			RegisterEntity(entityIndex);
	}

	void EditorWindow::OnLayerDoubleClicked(QListWidgetItem* item)
	{
		std::size_t layerIndex = static_cast<std::size_t>(item->data(Qt::UserRole).value<qulonglong>());

		auto& layer = m_workingMap.GetLayer(layerIndex);

		LayerInfo layerInfo;
		layerInfo.backgroundColor = layer.backgroundColor;
		layerInfo.depth = layer.depth;
		layerInfo.name = layer.name;

		LayerInfoDialog* layerInfoDialog = new LayerInfoDialog(layerInfo, this);
		connect(layerInfoDialog, &QDialog::accepted, [this, layerInfoDialog, layerIndex, item]()
		{
			LayerInfo layerInfo = layerInfoDialog->GetLayerInfo();

			auto& layer = m_workingMap.GetLayer(layerIndex);

			layer.backgroundColor = layerInfo.backgroundColor;
			m_canvas->UpdateBackgroundColor(layer.backgroundColor);

			layer.depth = layerInfo.depth;

			bool resetItemName = false;
			if (layer.name != layerInfo.name)
			{
				layer.name = layerInfo.name;
				resetItemName = true;
			}

			if (resetItemName)
				item->setText(QString::fromStdString(layer.name));
		});

		layerInfoDialog->exec();
	}

	void EditorWindow::OnOpenMap()
	{
		QString mapFolder = QFileDialog::getExistingDirectory(this, QString(), QString(), QFileDialog::ShowDirsOnly);
		if (mapFolder.isEmpty())
			return;

		std::filesystem::path workingMapPath = mapFolder.toStdString();

		Map map;

		try
		{
			map = Map::LoadFromFolder(workingMapPath);
		}
		catch (const std::exception& e)
		{
			QMessageBox::critical(this, tr("Failed to open map"), tr("Failed to open map: %1").arg(e.what()), QMessageBox::Ok);
			return;
		}

		statusBar()->showMessage(tr("Map %1 loaded").arg(map.GetMapInfo().name.data()), 3000);
		UpdateWorkingMap(std::move(map), std::move(workingMapPath));
	}

	void EditorWindow::OnSaveMap()
	{
		if (m_workingMapPath.empty())
		{
			QDir mapFolder;
			std::filesystem::path workingPath;

			for (;;)
			{
				QString path = QFileDialog::getExistingDirectory(this, QString(), QString(), QFileDialog::ShowDirsOnly);
				if (path.isEmpty())
					return;

				mapFolder = path;
				if (!mapFolder.isEmpty())
				{
					QMessageBox::critical(this, tr("Folder not empty"), tr("Map folder must be empty"), QMessageBox::Ok);
					continue;
				}

				workingPath = mapFolder.path().toStdString();
				break;
			}

			if (!mapFolder.mkdir("assets"))
				QMessageBox::warning(this, tr("Failed to create folder"), tr("Failed to create assets subdirectory (is map folder read-only?)"), QMessageBox::Ok);

			if (!mapFolder.mkdir("scripts"))
				QMessageBox::warning(this, tr("Failed to create folder"), tr("Failed to create scripts subdirectory (is map folder read-only?)"), QMessageBox::Ok);

			m_workingMapPath = std::move(workingPath);
		}

		if (m_workingMap.Save(m_workingMapPath))
			statusBar()->showMessage(tr("Map saved"), 3000);
		else
		{
			QMessageBox::warning(this, tr("Failed to save map"), tr("Failed to save map (is map folder read-only?)"), QMessageBox::Ok);
			statusBar()->showMessage(tr("Failed to save map"), 5000);
		}
	}

	void EditorWindow::RegisterEditorConfig()
	{
		m_config.RegisterStringOption("Assets.EditorFolder");
	}

	void EditorWindow::RegisterEntity(std::size_t entityIndex)
	{
		assert(m_currentLayer.has_value());

		auto& layer = m_workingMap.GetLayer(*m_currentLayer);

		assert(entityIndex < layer.entities.size());
		const Map::Entity& entity = layer.entities[entityIndex];

		QString entryName = QString::fromStdString(entity.entityType);
		if (!entity.name.empty())
			entryName = entryName % " (" % QString::fromStdString(entity.name) % ")";

		QListWidgetItem* item = new QListWidgetItem(entryName);
		item->setData(Qt::UserRole, qulonglong(entityIndex));

		Ndk::EntityId canvasId = m_canvas->CreateEntity(entity.entityType, entity.position, entity.rotation, entity.properties)->GetId();
		item->setData(Qt::UserRole + 1, canvasId);

		m_entityList->addItem(item);

		m_entityIndexes.emplace(canvasId, entityIndex);
	}
}
