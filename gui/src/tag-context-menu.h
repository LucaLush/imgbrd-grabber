#ifndef TAG_CONTEXT_MENU_H
#define TAG_CONTEXT_MENU_H

#include <QMenu>
#include <QStringList>
#include <QUrl>
#include "tags/tag.h"


class Profile;

class TagContextMenu : public QMenu
{
	Q_OBJECT

	public:
		TagContextMenu(const QString &tag, const QList<Tag> &allTags, const QUrl &browserUrl, Profile *profile, bool setImage = false, QWidget *parent = Q_NULLPTR);

	protected slots:
		void favorite();
		void setfavorite();
		void unfavorite();
		void viewitlater();
		void unviewitlater();
		void ignore();
		void unignore();
		void blacklist();
		void unblacklist();
		void openInNewTab();
		void openInNewWindow();
		void openInBrowser();
		void copyTagToClipboard();
		void copyAllTagsToClipboard();

	signals:
		void setFavoriteImage();
		void openNewTab();

	private:
		QString m_tag;
		QList<Tag> m_allTags;
		QUrl m_browserUrl;
		Profile *m_profile;
};

#endif // TAG_CONTEXT_MENU_H
