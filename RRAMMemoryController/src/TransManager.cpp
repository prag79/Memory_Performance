#include "TransManager.h"


	/** transManager allocation
	* @return void
	**/
	TransManager::gp_t* TransManager::allocate()
	{
		gp_t* ptr;
		if (free_list)
		{
			ptr = free_list->trans;
			empties = free_list;
			free_list = free_list->next;
		}
		else
		{
			ptr = new gp_t(this);
		}
		return ptr;
	}

	/** free paylaod
	* @return void
	**/
	void TransManager::free(gp_t* trans)
	{
		trans->reset(); // Delete auto extensions
		if (!empties)
		{
			empties = new access;
			empties->next = free_list;
			empties->prev = 0;
			if (free_list)
				free_list->prev = empties;
		}
		free_list = empties;
		free_list->trans = trans;
		empties = free_list->prev;
	}
