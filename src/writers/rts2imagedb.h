/* 
 * Database image access classes.
 * Copyright (C) 2003-2008 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*!file
 * Used for Image-DB interaction.
 *
 * Build on Rts2Image, persistently update image information in
 * database.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */

#ifndef __RTS2_IMAGEDB__
#define __RTS2_IMAGEDB__

#include "rts2image.h"
#include "../utils/rts2target.h"
#include "../utilsdb/rts2taruser.h"

// process_bitfield content
#define ASTROMETRY_PROC   0x01
// when & -> image should be in archive, otherwise it's in trash|que
// depending on ASTROMETRY_PROC
#define ASTROMETRY_OK     0x02
#define DARK_OK           0x04
#define FLAT_OK           0x08
// some error durring image operations occured, information in DB is unrealiable
#define IMG_ERR         0x8000

/**
 * Abstract class, representing image in DB.
 *
 * Rts2ImageSkyDb inherits from this class. This class is used to store any
 * non-sky images (mostly darks, flats and other callibration images).
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2ImageDb:public Rts2Image
{
	protected:
		virtual void initDbImage ();
		void reportSqlError (const char *msg);

		virtual int updateDB ()
		{
			return -1;
		}

		void getValueInd (const char *name, double &value, int &ind, char *comment = NULL);
		void getValueInd (const char *name, float &value, int &ind, char *comment = NULL);

	public:
		Rts2ImageDb (Rts2Image * in_image);
		Rts2ImageDb (Rts2Target * currTarget, Rts2DevClientCamera * camera, const struct timeval *expStart);
		Rts2ImageDb (const char *in_filename, bool verbose = true, bool readOnly = false);
		Rts2ImageDb (int in_obs_id, int in_img_id);
		Rts2ImageDb (long in_img_date, int in_img_usec, float in_img_exposure);

		int getOKCount ();

		virtual int saveImage ();

		virtual int renameImage (const char *new_filename);

		friend std::ostream & operator << (std::ostream & _os, Rts2ImageDb & img_db);
};

class Rts2ImageSkyDb:public Rts2ImageDb
{
	private:
		int updateAstrometry ();

		int processBitfiedl;
		inline int isCalibrationImage ();
		void updateCalibrationDb ();

	protected:
		virtual void initDbImage ();
		virtual int updateDB ();

	public:
		Rts2ImageSkyDb (Rts2Target * currTarget, Rts2DevClientCamera * camera, const struct timeval *expStartd);
		Rts2ImageSkyDb (const char *in_filename);
		//! Construct image from already existed Rts2ImageDb instance
		Rts2ImageSkyDb (Rts2Image * in_image);
		//! Construct image directly from DB (eg. retrieve all missing parameters)
		Rts2ImageSkyDb (int in_obs_id, int in_img_id);
		//! Construcy image from one database row..
		Rts2ImageSkyDb (int in_tar_id, int in_obs_id, int in_img_id,
			char in_obs_subtype, long in_img_date, int in_img_usec,
			float in_img_exposure, float in_img_temperature,
			const char *in_img_filter, float in_img_alt,
			float in_img_az, const char *in_camera_name,
			const char *in_mount_name, bool in_delete_flag,
			int in_process_bitfield, double in_img_err_ra,
			double in_img_err_dec, double in_img_err, const char *_img_path);
		virtual ~ Rts2ImageSkyDb (void);

		virtual int toArchive ();
		virtual int toTrash ();

		virtual int saveImage ();
		virtual int deleteImage ();

		virtual bool haveOKAstrometry ()
		{
			return (processBitfiedl & ASTROMETRY_OK);
		}

		virtual bool isProcessed ()
		{
			return (processBitfiedl & ASTROMETRY_PROC);
		}

		virtual std::string getFileNameString ();

		virtual img_type_t getImageType ()
		{
			return IMGTYPE_OBJECT;
		}
};

template < class img > img * setValueImageType (img * in_image)
{
	const char *imgTypeText = "unknow";
	img *ret_i = NULL;
	// guess image type..
	if (in_image->getShutter () == SHUT_CLOSED)
	{
		ret_i = in_image;
		imgTypeText = "dark";
	}
	else if (in_image->getShutter () == SHUT_UNKNOW)
	{
		// that should not happen
		return in_image;
	}
	else if (in_image->getTargetType () == TYPE_FLAT)
	{
		ret_i = in_image;
		imgTypeText = "flat";
	}
	else
	{
		ret_i = new Rts2ImageSkyDb (in_image);
		delete in_image;
		imgTypeText = "object";
	}
	ret_i->setValue ("IMAGETYP", imgTypeText, "IRAF based image type");
	return ret_i;
}


template < class img > img * getValueImageType (img * in_image)
{
	char value[20];
	try
	{
		in_image->getValue ("IMAGETYP", value, 20);
	}
	catch (rts2image::KeyNotFound &er)
	{
		return in_image;
	}
	// switch based on IMAGETYPE
	if (!strcasecmp (value, "dark"))
	{
		return in_image;
	}
	else if (!strcasecmp (value, "flat"))
	{
		return in_image;
	}
	else if (!strcasecmp (value, "object"))
	{
		Rts2ImageSkyDb *skyI = new Rts2ImageSkyDb (in_image);
		delete in_image;
		return skyI;
	}
	// "zero" and "comp" are not used
	return in_image;
}
#endif							 /* ! __RTS2_IMAGEDB__ */
