/***************************************************************************
                          kdenlivedoc.h  -  description
                             -------------------
    begin                : Fri Feb 15 01:46:16 GMT 2002
    copyright            : (C) 2002 by Jason Wood
    email                : jasonwood@blueyonder.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KDENLIVEDOC_H
#define KDENLIVEDOC_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif 

#include <qobject.h>
#include <qstring.h>
#include <qvaluelist.h>
#include <qmap.h>

#include <kurl.h>

#include "avfilelist.h"
#include "doctrackbaselist.h"
#include "krender.h"
#include "rangelist.h"

// forward declaration of the Kdenlive classes
class KdenliveView;

/**	KdenliveDoc provides a document object for a document-view model.
  *
  * The KdenliveDoc class provides a document object that can be used in conjunction with the classes
  * KdenliveApp and KdenliveView to create a document-view model for standard KDE applications based on
  * KApplication and KMainWindow. Thereby, the document object is created by the KdenliveApp instance and
  * contains the document structure with the according methods for manipulation of the document data by 
  * KdenliveView objects. Also, KdenliveDoc contains the methods for serialization of the document data 
  * from and to files.
  * 
  * @author Source Framework Automatically Generated by KDevelop, (c) The KDevelop Team. 	
  * @version KDevelop version 1.2 code generation
  */

class KdenliveDoc : public QObject
{
  Q_OBJECT
  public:
    /** Constructor for the fileclass of the application */
    KdenliveDoc(QWidget *parent, const char *name=0);
    /** Destructor for the fileclass of the application */
    ~KdenliveDoc();

    /** adds a view to the document which represents the document contents. Usually this is your main view. */
    void addView(KdenliveView *view);
    /** removes a view from the list of currently connected views */
    void removeView(KdenliveView *view);
    /** returns if the document is modified or not. Use this to determine if your document needs saving by the
     * user on closing. */
    bool isModified(){ return m_modified; };
    /** "save modified" - asks the user for saving if the document is modified */
    bool saveModified();	
    /** deletes the document's contents */
    void deleteContents();
    /** initializes the document generally */
    bool newDocument();
    /** closes the acutal document */
    void closeDocument();
    /** loads the document by filename and format and emits the updateViews() signal */
    bool openDocument(const KURL& url, const char *format=0);
    /** saves the document under filename and format.*/	
    bool saveDocument(const KURL& url, const char *format=0);
    /** returns the KURL of the document */
    const KURL& URL() const;
    /** sets the URL of the document */
	  void setURL(const KURL& url);
		/** Returns the internal avFile list. */
		const AVFileList &avFileList();
		/** Insert an AVFile with the given url. If the file is already in the file list, return that instead. */
		AVFile *insertAVFile(const KURL &file);
  public slots:
    /** calls repaint() on all views connected to the document object and is called by the view by which the document has been changed.
     * As this view normally repaints itself, it is excluded from the paintEvent.
     */
    void slotUpdateAllViews(KdenliveView *sender);
	  /** Inserts an Audio/visual file into the project */
	  void slot_InsertAVFile(const KURL &file);
  	/** Adds a sound track to the project */
  	void addSoundTrack();
  	/** Adds an empty video track to the project */
  	void addVideoTrack();
  /** Inserts a list of clips into the document, updating the project accordingly. */
  void slot_insertClips(QPtrList<DocClipBase> clips);
  /** Given a drop event, inserts all contained clips into the project list, if they are not there already. */
  void slot_insertClips(QDropEvent *event);
  /** This slot occurs when the File properties for an AV File have been returned by the renderer.

The relevant AVFile can then be updated to the correct status. */
  void AVFilePropertiesArrived(QMap<QString, QString> properties);
 	
	 public:	
 		/** the list of the views currently connected to the document */
 		static QPtrList<KdenliveView> *pViewList;	
  	/** The number of frames per second. */
  	int m_framesPerSecond;
  	/** Holds a list of all tracks in the project. */
  	DocTrackBaseList m_tracks;
  /** A temporary, static renderer. This variable should be removed from the source
as swiftly as possible... */
  static KRender temporaryRenderer;
  	/** Returns the number of frames per second. */
  	int framesPerSecond();
  	/** Itterates through the tracks in the project. This works in the same way
			* as QPtrList::next(), although the underlying structures may be different. */
	  DocTrackBase * nextTrack();
  	/** Returns the first track in the project, and resets the itterator to the first track.
			*This effectively is the same as QPtrList::first(), but the underyling implementation
			* may change. */
	  DocTrackBase * firstTrack();
	  /** Returns the number of tracks in this project */
	  int numTracks();
  /** Returns a reference to the AVFile matching the  url. If no AVFile matching the given url is found, then one will be created. Either way, the reference count for the AVFile will be incremented by one, and the file will be returned. */
  AVFile * getAVFileReference(KURL url);
  /** Find and return the AVFile with the url specified, or return null is no file matches. */
  AVFile * findAVFile(const KURL &file);
  /** returns the Track which holds the given clip. If the clip does not
exist within the document, returns 0; */
  DocTrackBase * findTrack(DocClipBase *clip);
  /** Returns the track with the given index, or returns NULL if it does
not exist. */
  DocTrackBase * track(int track);
  /** Returns the index value for this track, or -1 on failure.*/
  int trackIndex(DocTrackBase *track);
  /** Creates an xml document that describes this kdenliveDoc. */
  QDomDocument toXML();
  /** Sets the modified state of the document, if this has changed, emits modified(state) */
  void setModified(bool state);
  /** Removes entries from the AVFileList which are unreferenced by any clips. */
  void cleanAVFileList();
  /** Finds and removes the specified avfile from the document. If there are any
		clips on the timeline which use this clip, then they will be deleted as well.
		Emits AVFileList changed if successful. */
  void deleteAVFile(AVFile *file);
  /** Moves the currectly selected clips by the offsets specified, or returns false if this
is not possible. */
  bool moveSelectedClips(GenTime startOffset, int trackOffset);
  /** Returns a scene list generated from the current document. */
  QDomDocument generateSceneList();
  /** Renders the current document timeline to the specified url. */
  void renderDocument(const KURL &url);

  private:
    /** the modified flag of the current document */
    bool m_modified;
    KURL m_doc_url;

		/** List of all video and audio files within this project */
	AVFileList m_fileList;  	
  /** This renderer is for multipurpose use, such as background rendering, and for
	getting the file properties of the various AVFiles. */
  KRender * m_render;
  /** The range of times in the timeline that are currently out of date in the scene list. This list is used
	to re-sync the scene list. */
  RangeList<GenTime> m_invalidSceneTimes;;
  /** This is the scenelist that get's passed from the KdenliveDoc to the renderer. */
  QDomDocument m_domSceneList;
	private: // Private methods
  	/** Adds a track to the project */
  	void addTrack(DocTrackBase *track);
  /** Parses the XML Dom Document elements to populate the KdenliveDoc. */
  void loadFromXML(QDomDocument &doc);
	signals: // Signals
  	/** This signal is emitted whenever tracks are added to or removed from the project. */
  	void trackListChanged();
 	  /** This is signal is emitted whenever the avFileList changes, either through the addition or removal of an AVFile, or when an AVFile changes. */
  	void avFileListUpdated();
	  /** Emitted when the modified state of the document changes. */
	  void modified(bool);
private slots: // Private slots
  /** Called when the document is modifed in some way. */
  void hadBeenModified();
};

#endif // KDENLIVEDOC_H
