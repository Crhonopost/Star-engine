# Star-engine



# Utile:

**ResourceManager**: gestion simple des ressources (textures par ex)
Il faut que la ressource à stocker implémente IResource, le chargement passe par ResourceManager::load. Si la ressource a déjà été chargée, elle sera retournée pour éviter d'avoir des copies