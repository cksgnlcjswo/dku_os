/*
*	Operating System Lab
*	    Lab2 (Synchronization)
*	    Student id : 32161149
*	    Student name : kim chan hwi
*
*   lab2_bst.c :
*       - thread-safe bst code.
*       - coarse-grained, fine-grained lock code
*
*   Implement thread-safe bst for coarse-grained version and fine-grained version.
*/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <string.h>
#include "lab2_sync_types.h"

void inorder(lab2_node *root) {
	if(root) {
	inorder(root->left);
	//printf("%d ",root->key);
	inorder(root->right);
	}
}

int lab2_node_print_inorder(lab2_tree *tree) {
	inorder(tree->root);	
}

lab2_tree *lab2_tree_create() {
    lab2_tree *tmp=(lab2_tree *)malloc(sizeof(lab2_tree));
    tmp->root=NULL;
    pthread_mutex_init(&tmp->tree_lock,NULL);
    return tmp;
}

lab2_node *lab2_node_create(int key) {
    lab2_node *tmp=(lab2_node *)malloc(sizeof(lab2_node));
    tmp->left=tmp->right=NULL;
    tmp->key=key;
    pthread_mutex_init(&tmp->node_lock,NULL);
    return tmp;
}

int lab2_node_insert(lab2_tree *tree, lab2_node *new_node) {
   lab2_node *cur=tree->root;
   lab2_node *pre=NULL;
    
   if(!cur) { 
	   tree->root=new_node;
	   return 0;
   }
   while(cur) {	 //find appropriate position
	if(new_node->key==cur->key) return -1;    //when there exist same key 
	pre=cur;
	if(new_node->key < cur->key) cur=cur->left;
        else if(new_node->key > cur->key) cur=cur->right;	
   }
			/*now cur is null*/	
   if(new_node->key < pre->key)
     pre->left=new_node;
   else
     pre->right=new_node;
return 0;
}

int lab2_node_insert_cg(lab2_tree *tree, lab2_node *new_node) {   
   pthread_mutex_lock(&tree->tree_lock);	
   lab2_node *cur=tree->root;
   lab2_node *pre=NULL;
   int flag=0;

   if(!cur) { 
	tree->root=new_node;
	flag=1;
   }

    while(!flag&&cur) {         //find appropriate position        
      if(new_node->key==cur->key) {             //when there exist same key	
        flag=1;
	break;
      }
        pre=cur;
	if(new_node->key < cur->key) cur=cur->left;
        else if(new_node->key > cur->key) cur=cur->right;
   }
    	if(!flag) {
            if(new_node->key < pre->key)
              pre->left=new_node;
            else
              pre->right=new_node; 
	}
pthread_mutex_unlock(&tree->tree_lock);
return flag==1 ? -1 : 0;
}

int lab2_node_insert_fg(lab2_tree *tree, lab2_node *new_node) {
   if(!tree->root) {	//only first thread execute this instruction
	tree->root=new_node;
	return 0;
   }

   lab2_node *cur=tree->root;
   lab2_node *pre=NULL;
   
   pthread_mutex_lock(&cur->node_lock);
   
   while(cur) {         //find appropriate position
	   if(new_node->key==cur->key) {     //when there exist same key       
		if(pre) pthread_mutex_unlock(&pre->node_lock);
	   	pthread_mutex_unlock(&cur->node_lock);
		   return -1;
	   }

	if(pre) pthread_mutex_unlock(&pre->node_lock);
        pre=cur;
	if(new_node->key < cur->key) cur=cur->left;
        else if(new_node->key > cur->key) cur=cur->right;
        if(cur) pthread_mutex_lock(&cur->node_lock);
   }
   
   if(new_node->key < pre->key)
     pre->left=new_node;
   else
     pre->right=new_node;     	  
     pthread_mutex_unlock(&pre->node_lock);	 
return 0; 
}

int lab2_node_remove(lab2_tree *tree, int key) {
    
    if(!tree->root) return -1;
    
    lab2_node *cur=tree->root;
    lab2_node *pre=NULL;

    while(cur) {
	if(cur->key==key) break;
	pre=cur;
	if(cur->key < key) cur=cur->right;
	else if(cur->key > key) cur=cur->left;
    }

    if(!cur) return -1;		//there is no key in bst		
	
    /*pre is parent of cur ,cur is pointing node to be deleted*/
    if(cur->left==NULL && cur->right==NULL) {	//no child		
	if(pre) {
	  if(pre->left==cur)
	    pre->left=NULL;
	  else
	    pre->right=NULL;
	}
    	
	else
	  tree->root=NULL;
    free(cur);
    }

    else if(cur->left==NULL || cur->right==NULL) {	//one child
	if(pre) {
	  if(pre->left==cur) {
	    if(cur->left==NULL)
	      pre->left=cur->right;	    
	    else
	      pre->left=cur->left;
	  }  
	  else {	//pre->right==cur
	    if(cur->left==NULL)
	      pre->right=cur->right;
	    else
	      pre->right=cur->left;	    
	  }
	}
        else	//when pre is NULL
	tree->root=(cur->left!=NULL) ? cur->left : cur->right;	
    free(cur);
    }

    else {		//two child
    lab2_node *sub_min_pre=NULL;
    lab2_node *sub_min=cur->right;

    while(sub_min->left) {	//find a min key in the right subtree 
	sub_min_pre=sub_min;
	sub_min=sub_min->left;
    }    
    
    cur->key=sub_min->key;

    if(!sub_min_pre)		//cur is root
      cur->right=sub_min->right;
    else    
    sub_min_pre->left=sub_min->right;	//there may exist right child on sub_min or null
 
    free(sub_min);
    }
return 1;
}

int lab2_node_remove_cg(lab2_tree *tree, int key) {
     		
     if(!tree->root) return -1;

     lab2_node *cur=tree->root;
     lab2_node *pre=NULL;
	
     pthread_mutex_lock(&tree->tree_lock);

     while(cur) {
        if(cur->key==key) break;
        pre=cur;
        if(cur->key < key) cur=cur->right;
        else if(cur->key > key) cur=cur->left;
     }
	
    if(!cur) {
      pthread_mutex_unlock(&tree->tree_lock);
      return -1;
    }
    
    if(cur->left==NULL && cur->right==NULL) {   //no child              
        if(pre) {
          if(pre->left==cur)
            pre->left=NULL;
          else
            pre->right=NULL;
        }

        else
          tree->root=NULL;
    free(cur);
    }

    else if(cur->left==NULL || cur->right==NULL) {      //one child
      if(pre) {
        if(pre->left==cur) {
          if(cur->left==NULL)
            pre->left=cur->right;
          else
              pre->left=cur->left;
        }
        else {        //pre->right==cur
          if(cur->left==NULL)
            pre->right=cur->right;
          else
            pre->right=cur->left;
        }
      }
      else    //when pre is NULL
      tree->root=(cur->left!=NULL) ? cur->left : cur->right;
    free(cur);
    }

    else {              //two child
    lab2_node *sub_min_pre=NULL;
    lab2_node *sub_min=cur->right;

    while(sub_min->left) {      //find a max node in the right subtree 
        sub_min_pre=sub_min;
        sub_min=sub_min->left;
    }

    cur->key=sub_min->key;

    if(!sub_min_pre)
      cur->right=sub_min->right;
    else
      sub_min_pre->left=sub_min->right;    //there may exist right child on sub_min

    free(sub_min);
    }
    pthread_mutex_unlock(&tree->tree_lock);
return 1;
}

int lab2_node_remove_fg(lab2_tree *tree, int key) {
    if(!tree->root) return -1;	    

    pthread_mutex_lock(&tree->root->node_lock);

    lab2_node *cur=tree->root;
    lab2_node *pre=NULL;

    while(cur) {
        if(cur->key==key) break;
	if(pre) pthread_mutex_unlock(&pre->node_lock);
	
	pre=cur;
        if(cur->key < key) {
	  cur=cur->right;
	  if(cur)  pthread_mutex_lock(&cur->node_lock);	
	  }
	
        else if(cur->key > key) {
	  cur=cur->left;
	  if(cur) pthread_mutex_lock(&cur->node_lock);		
        }
   }

    if(!cur) {		//there is no key in tree
      if(pre) pthread_mutex_unlock(&pre->node_lock);  
    return -1;
    }

    if(cur->left==NULL && cur->right==NULL) {   //no child              
	if(pre) {
          if(pre->left==cur)
            pre->left=NULL;
          else
            pre->right=NULL;
	  pthread_mutex_unlock(&cur->node_lock);
	  lab2_node_delete(cur);
	  pthread_mutex_unlock(&pre->node_lock);
	}

        if(!pre) {
	tree->root=NULL;
	pthread_mutex_unlock(&cur->node_lock);	
    	lab2_node_delete(cur);
	}
    }

    else if(cur->left==NULL || cur->right==NULL) {      //one child
	 if(pre) {
           if(pre->left==cur) {
             if(cur->left==NULL)
               pre->left=cur->right;
             else
               pre->left=cur->left;
           }
	
          else {        //pre->right==cur
            if(cur->left==NULL)
              pre->right=cur->right;
            else
              pre->right=cur->left;
          }
	  pthread_mutex_unlock(&cur->node_lock);	
	  lab2_node_delete(cur);
	  pthread_mutex_unlock(&pre->node_lock); 
	 }

        if(!pre) {     //when pre is NULL, root node must not be deleted because of node_lock. therefore copy must be done
	  if(cur->left==NULL) {		
	    lab2_node *tmp=cur->right;
	    cur->left=tmp->left;		
	    cur->key=tmp->key;
	    cur->right=tmp->right;
	    lab2_node_delete(tmp);
	    pthread_mutex_unlock(&cur->node_lock);
	  }
	  else {
	    lab2_node *tmp=cur->left;
	    cur->right=tmp->right;
	    cur->key=tmp->key;
	    cur->left=tmp->left;
	    lab2_node_delete(tmp);
	    pthread_mutex_unlock(&cur->node_lock);
	  }  	 	
	}	
    }

    else {              //two child
    lab2_node *sub_min_pre=NULL;
    lab2_node *sub_min=cur->right;
    
    pthread_mutex_lock(&sub_min->node_lock);
    
    while(sub_min->left) {      //find a max node in the right subtree 
	if(sub_min_pre) pthread_mutex_unlock(&sub_min_pre->node_lock);
	  	
	sub_min_pre=sub_min;
	sub_min=sub_min->left;
	pthread_mutex_lock(&sub_min->node_lock);	
    }

    cur->key=sub_min->key;

    if(!sub_min_pre) 
      cur->right=sub_min->right;

    else 
      sub_min_pre->left=sub_min->right;    //there may exist right child on sub_min
      
    if(pre) pthread_mutex_unlock(&pre->node_lock); 
    
    pthread_mutex_unlock(&sub_min->node_lock);
    lab2_node_delete(sub_min);	    
   if(sub_min_pre) pthread_mutex_unlock(&sub_min_pre->node_lock);
	    
    pthread_mutex_unlock(&cur->node_lock);    
}
return 1;
}

void lab2_tree_delete(lab2_tree *tree) {
	while(tree->root)  	//remove all nodes
	lab2_node_remove(tree,tree->root->key);
	free(tree);
}

void lab2_node_delete(lab2_node *node) {
	node->left=node->right=NULL;
        node->key=0;
	free(node);	
}

