//
// Created by Sijia Chen on 2020/5/5.
//

#ifndef SIPHON_MESSAGE_DICTIONARY_H
#define SIPHON_MESSAGE_DICTIONARY_H

#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <stdint.h>

namespace siphon {
    namespace util {

        typedef size_t DictionaryHashCode; // type of the hash code used for the dictionary

        // Define callback functions for necessary operations used for the dictionary
        typedef DictionaryHashCode ( * DictionaryHashCallback )( const void * ); // btain hash code
        typedef bool               ( * DictionaryKeyCompareCallback )( const void *, const void * ); // compare the key
        typedef const void       * ( * DictionaryRetainCallback  )( const void * ); // retain the given input
        typedef void               ( * DictionaryReleaseCallback )( const void * ); // free the memory
        typedef void               ( * DictionaryPrintCallback )( FILE *, const void * ); // print out the information

        // A struct for defining call back functions for components used in the dictionary
        typedef struct
        {
            DictionaryHashCallback          kHash;
            DictionaryKeyCompareCallback    kCompare;
            DictionaryRetainCallback        kRetain;
            DictionaryRetainCallback        vRetain;
            DictionaryReleaseCallback       kRelease;
            DictionaryReleaseCallback       vRelease;
            DictionaryPrintCallback         kPrint;
            DictionaryPrintCallback         vPrint;
        } DictionaryCallbacks;


        struct DictionaryItem
        {
            struct DictionaryItem * next;
            const void            * key;
            const void            * value;
        };

        // Main structure of the dictionary
        struct Dictionary
        {
            struct DictionaryItem ** items; // the item in the dictionary
            size_t                   count; // number of items
            size_t                   size; // size of the dictionary
            DictionaryCallbacks      callbacks; // necessary callback functions
        };


        #define DICTIONARY_HASH_MULTIPLIER  ( 31 ) // the constant value for the hash multiply factor
        #define DICTIONARY_MAX_LOAD_FACTOR  (  1 ) // maximum items contained in the dictionary is size*DICTIONARY_MAX_LOAD_FACTOR
        #define DICTIONARY_GROWTH_FACTOR    (  2 ) // used for extending the dictionary by DICTIONARY_GROWTH_FACTOR


        typedef struct Dictionary * DictionaryRef;


        // Creates a dictionary using dynamic size
        //
        // This function is used to create a dictionary, it allocates memory for a buffer that can hold
        // the requested number of items.
        //
        // size: The size of dictionary,
        // callbacks: A pointer to DictionaryCallbacks struct that defines all necessary callback functions
        //
        // If a dictionary is created succesfully, returns a pointer to the
        // structure. NULL is returned if something fails.
        //
        DictionaryRef DictionaryCreate( size_t size, DictionaryCallbacks * callbacks );

        // Deletes a dictionary
        //
        // This function is used to delete a dictionary, it free all the memory occupied by this dictionary.
        //
        // dictionary_ref: The dictionary needed to be release,
        //
        // No return.
        //
        void DictionaryRelease( DictionaryRef dictionary_ref );

        // Clears a dictionary
        //
        // This function is used to clear a dictionary. It removes all items in this dictionary.
        //
        // dictionary_ref: The dictionary needed to be cleaned,
        //
        // No return.
        //
        void DictionaryClear( DictionaryRef dictionary_ref );

        // Inserts an item to the dictionary
        //
        // This function is used to insert the item to the dictionary. It makes the dictionary create a item with the given
        // kye and value.
        //
        // dictionary_ref: The dictionary needed to be inserted,
        // key: the key that is inserted to the item
        // value: the value that is inserted to the item
        //
        // No return.
        //
        void DictionaryInsert( DictionaryRef dictionary_ref, const void * key, const void * value );

        // Removes an item from the dictionary
        //
        // This function is used to remove an item from the dictionary. It makes the dictionary delete an item that has the given
        // kye.
        //
        // dictionary_ref: The dictionary needed to be removed,
        // key: the key that is required to be removed from the dictionary
        //
        // No return.
        //
        void DictionaryRemove( DictionaryRef dictionary_ref, const void * key );

        // Detects whether the key exists in the dictionary
        //
        // This function is used to detect whether the given key exists in the dictionary.
        //
        // dictionary_ref: The dictionary needed to be searched,
        // key: the key needed to be searched.
        //
        // Return True if searched key is in the dictionary.
        //
        bool DictionaryKeyExists( DictionaryRef dictionary_ref, const void * key );

        // Gets the value of the given key in the dictionary
        //
        // This function is used to extract the value of the searched key from in dictionary.
        //
        // dictionary_ref: The dictionary needed to be searched,
        // key: the key whose corresponding value will be returned .
        //
        // Return A pointer to this value if the key is in the dictionary. Otherwise, it returns NULL.
        //
        const void  * DictionaryGetValue( DictionaryRef dictionary_ref, const void * key );

        // Gets the key whose value is the maximum one in the dictionary
        //
        // This function is used to obtain the key that corresponds to the maximum value of the dictionary.
        //
        // dictionary_ref: The dictionary needed to be searched,
        // obtained_key: a pointer points to a memory that the content of the obtained key will be copy to.
        // key_size: size of the key that will be copied to the obtained_key
        // Return A pointer to this value if the key is in the dictionary. Otherwise, it returns NULL.
        //
        void GetKeyWithMaximumValue(DictionaryRef dictionary_ref, void * obtained_key, int key_size);

        // Prints out all the items with (key, value) in the given dictionary
        void DictionaryPrint( DictionaryRef dictionary_ref );

        // Swaps the content of two dictionaries
        void DictionarySwap( DictionaryRef destination_dictionary_ref, DictionaryRef source_dictionary_ref );

        // Resize the dictionary by the given size.
        //
        // This function is used to extend the size the dictionary by DICTIONARY_GROWTH_FACTOR * size.
        //
        // dictionary_ref: The dictionary needed to be extended,
        // size: the basic size needed to be extended for the dictionary.

        // No return.
        //
        void DictionaryResize( DictionaryRef dictionary_ref, size_t size );

        // The callback functions which will be used in the operations of the dictionary
        DictionaryCallbacks DictionaryStandardStringCallbacks( void );
        DictionaryHashCode  DictionaryHashStringCallback( const void * key );
        bool                DictionaryCompareStringCallback( const void * k1, const void * k2 );
        static struct DictionaryItem * DictionaryGetItem( DictionaryRef dictionary_ref, const void * key );

    }
}


#endif //SIPHON_MESSAGE_DICTIONARY_H
