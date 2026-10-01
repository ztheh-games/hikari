#ifndef HIKARI_CLIENT_GAME_OBJECT_ITEMFACTORY
#define HIKARI_CLIENT_GAME_OBJECT_ITEMFACTORY

#include <memory>
#include <string>
#include <unordered_map>


namespace hikari {
    class CollectableItem;

    class ItemFactory {
    private:
        //
        // Fields
        //
        std::unordered_map<std::string, std::shared_ptr<CollectableItem>> prototypeRegistry;

    public:
        //
        // Constructor
        //
        ItemFactory();
        virtual ~ItemFactory();

        //
        // Methods
        //
        std::shared_ptr<CollectableItem> createItem(const std::string& itemType);

        void registerPrototype(const std::string & prototypeName, const std::shared_ptr<CollectableItem> & instance);
    };

} // hikari

#endif // HIKARI_CLIENT_GAME_OBJECT_ITEMFACTORY