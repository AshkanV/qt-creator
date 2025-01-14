// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "axivionprojectsettings.h"

#include "axivionplugin.h"
#include "axivionsettings.h"
#include "axiviontr.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettingswidget.h>

#include <solutions/tasking/tasktreerunner.h>

#include <utils/infolabel.h>
#include <utils/qtcassert.h>

#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace Tasking;
using namespace Utils;

namespace Axivion::Internal {

const char PSK_PROJECTNAME[] = "Axivion.ProjectName";

// AxivionProjectSettingsHandler

class AxivionProjectSettingsHandler : public QObject
{
public:
    AxivionProjectSettings *projectSettings(ProjectExplorer::Project *project)
    {
        auto &settings = m_axivionProjectSettings[project];
        if (!settings)
            settings = new AxivionProjectSettings(project);
        return settings;
    }

    void destroy()
    {
        qDeleteAll(m_axivionProjectSettings);
        m_axivionProjectSettings.clear();
    }

    QHash<ProjectExplorer::Project *, AxivionProjectSettings *> m_axivionProjectSettings;
};

static AxivionProjectSettingsHandler &projectSettingsHandler()
{
    static AxivionProjectSettingsHandler theProjectSettingsHandler;
    return theProjectSettingsHandler;
}

// AxivionProjectSettings

AxivionProjectSettings::AxivionProjectSettings(ProjectExplorer::Project *project)
    : m_project{project}
{
    load();
    connect(project, &ProjectExplorer::Project::settingsLoaded,
            this, &AxivionProjectSettings::load);
    connect(project, &ProjectExplorer::Project::aboutToSaveSettings,
            this, &AxivionProjectSettings::save);
}

AxivionProjectSettings *AxivionProjectSettings::projectSettings(ProjectExplorer::Project *project)
{
    return projectSettingsHandler().projectSettings(project);
}

void AxivionProjectSettings::destroyProjectSettings()
{
    projectSettingsHandler().destroy();
}

void AxivionProjectSettings::load()
{
    m_dashboardProjectName = m_project->namedSettings(PSK_PROJECTNAME).toString();
}

void AxivionProjectSettings::save()
{
    m_project->setNamedSettings(PSK_PROJECTNAME, m_dashboardProjectName);
}

// AxivionProjectSettingsWidget

class AxivionProjectSettingsWidget : public ProjectExplorer::ProjectSettingsWidget
{
public:
    explicit AxivionProjectSettingsWidget(ProjectExplorer::Project *project);

private:
    void fetchProjects();
    void onSettingsChanged();
    void linkProject();
    void unlinkProject();
    void updateUi();
    void updateEnabledStates();

    AxivionProjectSettings *m_projectSettings = nullptr;
    QLabel *m_linkedProject = nullptr;
    QTreeWidget *m_dashboardProjects = nullptr;
    QPushButton *m_fetchProjects = nullptr;
    QPushButton *m_link = nullptr;
    QPushButton *m_unlink = nullptr;
    Utils::InfoLabel *m_infoLabel = nullptr;
    TaskTreeRunner m_taskTreeRunner;
};

AxivionProjectSettingsWidget::AxivionProjectSettingsWidget(ProjectExplorer::Project *project)
    : m_projectSettings(projectSettingsHandler().projectSettings(project))
{
    setUseGlobalSettingsCheckBoxVisible(false);
    setUseGlobalSettingsLabelVisible(true);
    setGlobalSettingsId("Axivion.Settings.General"); // FIXME move id to constants
    // setup ui
    auto verticalLayout = new QVBoxLayout(this);
    verticalLayout->setContentsMargins(0, 0, 0, 0);

    m_linkedProject = new QLabel(this);
    verticalLayout->addWidget(m_linkedProject);

    m_dashboardProjects = new QTreeWidget(this);
    m_dashboardProjects->setHeaderHidden(true);
    m_dashboardProjects->setRootIsDecorated(false);
    verticalLayout->addWidget(new QLabel(Tr::tr("Dashboard projects:")));
    verticalLayout->addWidget(m_dashboardProjects);

    m_infoLabel = new Utils::InfoLabel(this);
    m_infoLabel->setVisible(false);
    verticalLayout->addWidget(m_infoLabel);

    auto horizontalLayout = new QHBoxLayout;
    horizontalLayout->setContentsMargins(0, 0, 0, 0);
    m_fetchProjects = new QPushButton(Tr::tr("Fetch Projects"));
    horizontalLayout->addWidget(m_fetchProjects);
    m_link = new QPushButton(Tr::tr("Link Project"));
    m_link->setEnabled(false);
    horizontalLayout->addWidget(m_link);
    m_unlink = new QPushButton(Tr::tr("Unlink Project"));
    m_unlink->setEnabled(false);
    horizontalLayout->addWidget(m_unlink);
    verticalLayout->addLayout(horizontalLayout);

    connect(m_dashboardProjects, &QTreeWidget::itemSelectionChanged,
            this, &AxivionProjectSettingsWidget::updateEnabledStates);
    connect(m_fetchProjects, &QPushButton::clicked,
            this, &AxivionProjectSettingsWidget::fetchProjects);
    connect(m_link, &QPushButton::clicked,
            this, &AxivionProjectSettingsWidget::linkProject);
    connect(m_unlink, &QPushButton::clicked,
            this, &AxivionProjectSettingsWidget::unlinkProject);
    connect(&settings(), &AspectContainer::changed,
            this, &AxivionProjectSettingsWidget::onSettingsChanged);

    updateUi();
}

void AxivionProjectSettingsWidget::fetchProjects()
{
    m_dashboardProjects->clear();
    m_fetchProjects->setEnabled(false);
    m_infoLabel->setVisible(false);

    const auto onDashboardInfo = [this](const expected_str<DashboardInfo> &info) {
        if (!info) {
            m_infoLabel->setText(info.error());
            m_infoLabel->setType(Utils::InfoLabel::Error);
            m_infoLabel->setVisible(true);
        } else {
            for (const QString &project : info->projects)
                new QTreeWidgetItem(m_dashboardProjects, {project});
        }
        updateEnabledStates();
    };

    m_taskTreeRunner.start(dashboardInfoRecipe(onDashboardInfo));
}

void AxivionProjectSettingsWidget::onSettingsChanged()
{
    m_dashboardProjects->clear();
    m_infoLabel->setVisible(false);
    updateUi();
}

void AxivionProjectSettingsWidget::linkProject()
{
    const QList<QTreeWidgetItem *> selected = m_dashboardProjects->selectedItems();
    QTC_ASSERT(selected.size() == 1, return);

    const QString projectName = selected.first()->text(0);
    m_projectSettings->setDashboardProjectName(projectName);
    updateUi();
    fetchProjectInfo(projectName);
}

void AxivionProjectSettingsWidget::unlinkProject()
{
    QTC_ASSERT(!m_projectSettings->dashboardProjectName().isEmpty(), return);

    m_projectSettings->setDashboardProjectName({});
    updateUi();
    fetchProjectInfo({});
}

void AxivionProjectSettingsWidget::updateUi()
{
    const QString projectName = m_projectSettings->dashboardProjectName();
    if (projectName.isEmpty())
        m_linkedProject->setText(Tr::tr("This project is not linked to a dashboard project."));
    else
        m_linkedProject->setText(Tr::tr("This project is linked to \"%1\".").arg(projectName));
    updateEnabledStates();
}

void AxivionProjectSettingsWidget::updateEnabledStates()
{
    const bool hasDashboardSettings = !settings().server.dashboard.isEmpty()
            && !settings().server.token.isEmpty();
    const bool linked = !m_projectSettings->dashboardProjectName().isEmpty();
    const bool linkable = m_dashboardProjects->topLevelItemCount()
            && !m_dashboardProjects->selectedItems().isEmpty();

    m_fetchProjects->setEnabled(hasDashboardSettings);
    m_link->setEnabled(!linked && linkable);
    m_unlink->setEnabled(linked);

    if (!hasDashboardSettings) {
        m_infoLabel->setText(Tr::tr("Incomplete or misconfigured settings."));
        m_infoLabel->setType(Utils::InfoLabel::NotOk);
        m_infoLabel->setVisible(true);
    }
}

class AxivionProjectPanelFactory : public ProjectExplorer::ProjectPanelFactory
{
public:
    AxivionProjectPanelFactory()
    {
        setPriority(250);
        setDisplayName(Tr::tr("Axivion"));
        setCreateWidgetFunction([](ProjectExplorer::Project *project) {
            return new AxivionProjectSettingsWidget(project);
        });
    }
};

void AxivionProjectSettings::setupProjectPanel()
{
    static AxivionProjectPanelFactory theAxivionProjectPanelFactory;
}

} // Axivion::Internal
